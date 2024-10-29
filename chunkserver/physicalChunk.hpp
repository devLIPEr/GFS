#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>

#include "../common/util.hpp"

namespace std
{
    class physicalChunk
    {
    private:
        string path = "";
        string file = "";
        unsigned long long id = 0;
        int offset = 0;
        int size = 0;
        unsigned long long version = 0;
        string nextNode;
    public:
        physicalChunk();
        physicalChunk(unsigned long long id, string path, string file, string nextNode);
        ~physicalChunk();

        int getOffset();
        void setOffset(int offset);

        string getLocation();
        void setPath(string path);
        void setFile(string file);

        string getNextNode();

        unsigned short write(string text, int offset);
        string read(int number, int start);
    };
    
    physicalChunk::physicalChunk()
    {
    }
    
    physicalChunk::physicalChunk(unsigned long long id, string path, string file, string nextNode = 0)
    {
        this->id = id;
        this->path = (path[path.length()-1] != '/') ? path + '/' : path;
        this->file = file;
        this->nextNode = nextNode;

        filesystem::create_directories(this->path);

        ofstream outFile(this->getLocation());
        outFile.close();
    }
    
    physicalChunk::~physicalChunk()
    {
    }

    string physicalChunk::getLocation(){
        return this->path + this->file;
    }

    void physicalChunk::setPath(string path){
        this->path = path;
    }

    void physicalChunk::setFile(string file){
        this->file = file;
    }

    int physicalChunk::getOffset(){
        return this->offset;
    }

    void physicalChunk::setOffset(int offset){
        this->offset = offset;
    }

    string physicalChunk::getNextNode(){
        return this->nextNode;
    }

    unsigned short physicalChunk::write(string text, int offset = -1){
        offset = (offset == -1) ? this->offset : offset;

        ofstream file(this->path + this->file, std::ios::in | std::ios::out);
        file.seekp(offset);
        int remainingSpace = 65535 - this->size;
        this->version++;

        rpc::client chunkClient(this->nextNode, util::chunkChunkPort);

        if(text.length() <= remainingSpace){
            file << text;
            try{
                if(this->nextNode != "0.0.0.0"){
                    // call write to the next node
                    chunkClient.call("write", this->id, text, this->offset);
                }
            }catch(const std::exception& e){
                std::cerr << e.what() << '\n';
            }
            this->offset += text.length();
            this->size += text.length();
            file.close();
        }else{
            file << text.substr(0, remainingSpace);
            try{
                if(this->nextNode != "0.0.0.0"){
                    // call write to the next node
                    chunkClient.call("write", this->id, text, this->offset);
                }
            }catch(const std::exception& e){
                std::cerr << e.what() << '\n';
            }
            this->offset = 65535;
            this->size = 65535;
            file.close();
            return remainingSpace;
        }
        return 0;
    }

    string physicalChunk::read(int number = -1, int start = 0){
        number = (number == -1) ? this->offset : number;
        ifstream file(this->getLocation());
        file.seekg(start);

        char *readed = new char[number];
        file.read(readed, number);
        file.close();

        return string(readed);
    }
} // namespace std
