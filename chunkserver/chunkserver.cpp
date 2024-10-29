// https://github.com/rpclib/rpclib/blob/master/examples/mandelbrot/mandelbrot_server.cc

#include <iostream>
#include <map>
#include <vector>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>
#include <variant>

#include <arpa/inet.h>
#include <cerrno>
#include <ifaddrs.h>
#include <net/if.h>
#include <string>
#include <string.h>
#include <sysexits.h>
#include <sys/socket.h>

#include "rpc/server.h"
#include "rpc/client.h"

#include "physicalChunk.hpp"

using namespace std;

#define BASE_DIR "./data"    // base data dir

map<unsigned long long, physicalChunk> chunks; // id to chunks map
unsigned long long spaceUsed = 0;

uint8_t createChunk(unsigned long long id, string path, string file, string nextNode = ""){
    cout << "Creating chunk " << id << " for " << path << ' ' << file << " nextNode = " << nextNode << '\n';
    if(chunks.find(id) == chunks.end()){
        physicalChunk c = physicalChunk(id, BASE_DIR+path, file, nextNode);
        chunks[id] = c;
        return 1;
    }
    return 0;
}

int writeChunk(unsigned long long id, string text, int offset = -1){
    cout << "Writing on chunk " << id << '\n';
    spaceUsed += text.length();
    return chunks[id].write(text, offset);
}

string readChunk(unsigned long long id, int start = 0, int number = -1){
    cout << "Reading on chunk " << id << " from " << start << " to " << start+number << '\n';
    return chunks[id].read(number, start);
}

int removeChunk(unsigned long long id){
    cout << "Deleting chunk " << id << " on " << chunks[id].getLocation() << '\n';
    if(chunks[id].getNextNode() != "0.0.0.0"){
        rpc::client client(chunks[id].getNextNode(), util::chunkChunkPort);

        try{
            client.call("remove", id);
        }catch(const std::exception& e){
            std::cerr << e.what() << '\n';
        }
    }

    remove(chunks[id].getLocation().c_str());
    chunks.erase(id);

    return 0;
}

void snapshot(unsigned long long id, string path, string file, string node = "", string nextNode = "", string text = ""){
    if(node == ""){
        filesystem::create_directories(path);
        if(nextNode != ""){
            filesystem::copy(chunks[id].getLocation().c_str(), path);
            chunks[id].setPath(path);
            chunks[id].setFile(file);
        }else{
            createChunk(id, path, file, nextNode);
            writeChunk(id, text);
        }
    }else{
        rpc::client client(node, util::chunkChunkPort);

        try{
            // this can be broken into multiple writes
            client.call("snapshot", id, path, file, 0, nextNode, text);
        }catch(const std::exception& e){
            std::cerr << e.what() << '\n';
        }
    }
}

// https://dev.to/fmtweisszwerg/cc-how-to-get-all-interface-addresses-on-the-local-device-3pki
string getIP(){
    struct ifaddrs* ptr_ifaddrs = nullptr;

    auto result = getifaddrs(&ptr_ifaddrs);
    if( result != 0 ){
        std::cout << "`getifaddrs()` failed: " << strerror(errno) << std::endl;

        return "";
    }

    for(
        struct ifaddrs* ptr_entry = ptr_ifaddrs;
        ptr_entry != nullptr;
        ptr_entry = ptr_entry->ifa_next
    ){
        std::string ipaddress_human_readable_form;
        std::string netmask_human_readable_form;

        std::string interface_name = std::string(ptr_entry->ifa_name);
        sa_family_t address_family = ptr_entry->ifa_addr->sa_family;
        if( address_family == AF_INET ){
            // IPv4

            // Be aware that the `ifa_addr`, `ifa_netmask` and `ifa_data` fields might contain nullptr.
            // Dereferencing nullptr causes "Undefined behavior" problems.
            // So it is need to check these fields before dereferencing.
            if( ptr_entry->ifa_addr != nullptr ){
                char buffer[INET_ADDRSTRLEN] = {0, };
                inet_ntop(
                    address_family,
                    &((struct sockaddr_in*)(ptr_entry->ifa_addr))->sin_addr,
                    buffer,
                    INET_ADDRSTRLEN
                );

                ipaddress_human_readable_form = std::string(buffer);
            }

            if( ptr_entry->ifa_netmask != nullptr ){
                char buffer[INET_ADDRSTRLEN] = {0, };
                inet_ntop(
                    address_family,
                    &((struct sockaddr_in*)(ptr_entry->ifa_netmask))->sin_addr,
                    buffer,
                    INET_ADDRSTRLEN
                );

                netmask_human_readable_form = std::string(buffer);
            }

            if(interface_name == "eth0" || interface_name == "enp0s3"){
                // return util::IP2number(util::fixIP(ipaddress_human_readable_form));
                return ipaddress_human_readable_form;
            }
            // std::cout << interface_name << ": IP address = " << ipaddress_human_readable_form << ", netmask = " << netmask_human_readable_form << std::endl;
        }
    }

    freeifaddrs(ptr_ifaddrs);
    return "0.0.0.0";
}

int main(int argc, char * argv[]){
    // Chunk 2 Chunk communication
    // functions only chunks can call in other chunks
    rpc::server chunk2chunkRPC(util::chunkChunkPort);
    chunk2chunkRPC.bind("write", &writeChunk);
    chunk2chunkRPC.bind("snapshot", &snapshot);
    chunk2chunkRPC.bind("remove", &removeChunk);
    chunk2chunkRPC.async_run(1); // start server on one other thread

    // Master 2 Chunk communication
    // functions only master can call in physicalChunk
    rpc::server master2chunk(util::masterChunkPort);
    master2chunk.bind("create", &createChunk);
    master2chunk.bind("remove", &removeChunk);
    master2chunk.async_run(1);

    // Client 2 Chunk communication
    // functions only client can call in physicalChunk
    rpc::server client2chunk(util::clientChunkPort);
    client2chunk.bind("write", &writeChunk);
    client2chunk.bind("read", &readChunk);
    client2chunk.async_run(1);

    mkdir(BASE_DIR, 0777);

    string ip;
    if(argc > 1){
        ip = string(argv[1]);
    }else{
        cout << "Insira o IP (X.X.X.X) do servidor mestre: ";
        cin >> ip;
    }

    rpc::client masterClient(ip, util::masterChunkPort);
    string myIP = getIP();
    unsigned long long free = (unsigned long long)filesystem::space(BASE_DIR).available;
    try{
        // cout << "Chunkserver's IP: " << util::number2IP(myIP) << '\n';
        cout << "Chunkserver's IP: " << myIP << '\n';
        masterClient.call("register", myIP, free);
    }catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }

    // as async_run is non-blocking, we need to wait here
    std::cout << "Press [ENTER] to exit the server." << std::endl;
    std::cin.ignore();
    std::cin.ignore();

    try{
        masterClient.call("unregister", myIP, free);
    }catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }

    return 0;
}