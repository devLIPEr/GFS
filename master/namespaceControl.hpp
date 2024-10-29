#include <map>
#include <vector>
#include <string>

#include "../common/chunk.hpp"
#include "../common/util.hpp"

namespace std
{
    class namespaceControl
    {
    private:
        string name;
        namespaceControl* parent = NULL;
        map<string, namespaceControl> files;
        vector<chunk> chunks;
    public:
        namespaceControl();
        namespaceControl(string name, namespaceControl* parent);
        ~namespaceControl();

        void print(string tab);
        namespaceControl* createFile(string path, string file);
        void addChunk(chunk c);
        namespaceControl* findChunk(string path, string file);
        chunk getChunk(string path, string file, int index);
        int removeChunk(int index);
        string getName(){return this->name;}
        vector<chunk> getChunks(){return this->chunks;}
    };

    namespaceControl::namespaceControl()
    {
    }
    namespaceControl::namespaceControl(string name, namespaceControl* parent)
    {
        this->name = name;
        this->parent = parent;
    }

    namespaceControl::~namespaceControl()
    {
    }

    namespaceControl* namespaceControl::createFile(string path, string file){
        if(path[path.length()-1] != '/'){
            path += '/';
        }
        vector<string> folders = util::split(path, '/');
        map<string, namespaceControl> *namespaceFiles = &this->files;
        namespaceControl *parent = this;
        for(string folder : folders){
            if(namespaceFiles->find(folder) != namespaceFiles->end()){
                namespaceFiles = &((*namespaceFiles)[folder].files);
            }else{
                (*namespaceFiles)[folder] = namespaceControl(folder, parent);
                parent = &((*namespaceFiles)[folder]);
                namespaceFiles = &((*namespaceFiles)[folder].files);
            }
        }
        if(namespaceFiles->find(file) != namespaceFiles->end()){
            return &((*namespaceFiles)[file]);
        }
        (*namespaceFiles)[file] = namespaceControl(file, parent);
        return &((*namespaceFiles)[file]);
    }

    void namespaceControl::print(string tab = " "){
        cout << tab << this->name << this->files.size() << '\n';
        tab += ' ';
        for(int i = 0; i < this->files.size(); i++){
            for(auto file : this->files){
                file.second.print(tab);
            }
        }
    }

    void namespaceControl::addChunk(chunk c){
        this->chunks.push_back(c);
    }

    namespaceControl* namespaceControl::findChunk(string path, string file){
        if(path[path.length()-1] != '/'){
            path += '/';
        }
        vector<string> folders = util::split(path, '/');
        map<string, namespaceControl> *namespaceFiles = &this->files;
        for(string folder : folders){
            if(namespaceFiles->find(folder) != namespaceFiles->end()){
                namespaceFiles = &((*namespaceFiles)[folder].files);
            }
        }
        if(namespaceFiles->find(file) != namespaceFiles->end()){
            return &((*namespaceFiles)[file]);
        }

        return new namespaceControl("", NULL);
    }

    chunk namespaceControl::getChunk(string path, string file, int index){
        namespaceControl *aux = this->findChunk(path, file);
        if(index < aux->getChunks().size())
            return aux->getChunks()[index];
        return chunk(0, "");
    }

    int namespaceControl::removeChunk(int index){
        this->chunks.erase(chunks.begin()+index);
        return this->chunks.size();
    }
} // namespace std