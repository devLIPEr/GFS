// https://static.googleusercontent.com/media/research.google.com/en//archive/gfs-sosp2003.pdf

#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>

#include "rpc/server.h"
#include "rpc/client.h"

#include "namespaceControl.hpp"
#include "meanHeap.hpp"

using namespace std;

#define NUM_REPLICAS 3 // number of replicas to be made

namespaceControl root = namespaceControl("/", NULL);

// map of chunkservers connected to master (ip, server)
map<string, chunkserver> servers;
meanHeap heap = meanHeap();
unsigned long long remainingSpace = 0;

int registerServer(string ip = "", unsigned long long freeSpace = 0){
    cout << "Server " << ip << " has " << freeSpace << " of free space\n";
    if(ip != ""){
        servers[ip] = chunkserver(ip, freeSpace);
        remainingSpace += freeSpace;
        heap.addHeap(servers[ip], remainingSpace/servers.size());
        return 1;
    }
    return 0;
}

int unregisterServer(string ip = "", unsigned long long freeSpace = 0){
    cout << "Server " << ip << " has disconnected loosing " << freeSpace << " of free space\n";
    if(ip != ""){
        remainingSpace -= freeSpace;
        servers.erase(ip);
        return 1;
    }
    return 0;
}

string chooseServer(){
    chunkserver server = heap.popHeap();
    cout << "Choosing server " << server.getIP() << ' ' << heap.size() << '\n';
    servers[server.getIP()].setFreeSpace(servers[server.getIP()].getFreeSpace()-65535); // max chunk size, approximated for simplicity
    remainingSpace -= 65535; // max chunk size, approximated for simplicity
    return server.getIP();
}

string createChunk(string path, string file){
    string serversArr[NUM_REPLICAS+1] = {""};
    for(int i = 0; i < NUM_REPLICAS; i++){
        serversArr[i] = chooseServer();
    }
    serversArr[NUM_REPLICAS] = "0.0.0.0";

    namespaceControl *fileNamespace = root.createFile(path, file);
    unsigned long long id = util::getChunkId();
    chunk c = chunk(id, serversArr[0]); // first server chosen is the primary chunkserver
    fileNamespace->addChunk(c);

    try{
        // link all servers in a chain
        for(int i = 0; i < NUM_REPLICAS; i++){
            rpc::client client(serversArr[i], util::masterChunkPort);
            client.call("create", id, path, file, serversArr[i+1]);
            heap.addHeap(servers[serversArr[i]], remainingSpace/servers.size());
        }
    }catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }

    cout << "Creating chunk for " << path << ' ' << file << " Chunk info " << c.getChunkId() << ' ' << c.getLocation() << '\n';
    return util::serializeChunk<chunk>(c);
}

string getChunk(string path, string file, int index){
    chunk c = root.getChunk(path, file, index);
    cout << "Getting chunk on " << path << ' ' << file << " on index " << index << " Chunk info " << c.getChunkId() << ' ' << c.getLocation() << '\n';
    return util::serializeChunk<chunk>(c);
}

unsigned long long removeChunk(string path, string file, int index){
    chunk c = root.getChunk(path, file, index);
    cout << "Deleting chunk on " << path << ' ' << file << " on index " << index << " Chunk info " << c.getChunkId() << ' ' << c.getLocation() << '\n';
    rpc::client client(c.getLocation(), util::masterChunkPort);

    try{
        client.call("remove", c.getChunkId());
    }catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }

    namespaceControl *fileNamespace = root.findChunk(path, file);
    unsigned long long size = fileNamespace->removeChunk(index);
    remainingSpace += 65535; // max chunk size, approximated for simplicity
    return size;
}

int main(){
    // Master 2 Chunk communication
    // functions only chunk can call on server
    rpc::server masterChunk(util::masterChunkPort);
    masterChunk.bind("register", &registerServer);
    masterChunk.bind("unregister", &unregisterServer);
    masterChunk.async_run(1);
    
    // Master 2 Client communication
    // functions only client can call con server
    rpc::server masterClient(util::clientMasterPort);
    masterClient.bind("create", &createChunk);
    masterClient.bind("get", &getChunk);
    masterClient.bind("remove", &removeChunk);
    masterClient.async_run(1);

    // as async_run is non-blocking, we need to wait here
    std::cout << "Press [ENTER] to exit the server." << std::endl;
    std::cin.ignore();

    return 0;
}