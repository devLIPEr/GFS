// https://stackoverflow.com/questions/15319561/how-to-implement-a-median-heap

#include <queue>

#include "chunkserver.hpp"

namespace std
{
    class meanHeap
    {
    private:
        priority_queue<chunkserver> minHeap; // elements bigger than mean
        priority_queue<chunkserver, vector<chunkserver>, greater<chunkserver>> maxHeap; // elements smaller than mean
        vector<string> servers;
    public:
        meanHeap();
        ~meanHeap();

        int size(){
            return minHeap.size() + maxHeap.size();
        }

        void addHeap(chunkserver server, double mean);
        chunkserver popHeap();
    };
    
    meanHeap::meanHeap()
    {
    }
    
    meanHeap::~meanHeap()
    {
    }
    
    void meanHeap::addHeap(chunkserver server, double mean){
       if(find(servers.begin(), servers.end(), server.getIP()) == servers.end()){
            if(server.getFreeSpace() < mean){
                maxHeap.push(server);
            }else{
                minHeap.push(server);
            }
            servers.push_back(server.getIP());
       }
    }
    
    chunkserver meanHeap::popHeap(){
        chunkserver server = chunkserver("0.0.0.0", 0);
        if(minHeap.size() > maxHeap.size()){
            server = minHeap.top();
            minHeap.pop();
            servers.erase(find(servers.begin(), servers.end(), server.getIP()));
        }else{
            if(maxHeap.size() > 0){
                server = maxHeap.top();
                maxHeap.pop();
            }else if(minHeap.size() > 0){
                server = minHeap.top();
                minHeap.pop();
            }
            servers.erase(find(servers.begin(), servers.end(), server.getIP()));
        }
        return server;
    }
} // namespace std
