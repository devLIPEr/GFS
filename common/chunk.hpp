namespace std
{
    class chunk
    {
    private:
        unsigned long long chunkId;
        string location;
    public:
        chunk();
        chunk(unsigned long long chunkId, string location);
        ~chunk();

        unsigned long long getChunkId();
        string getLocation();
        
        bool operator==(chunk const& other) const { return this->chunkId == other.chunkId; }
    };
    
    chunk::chunk()
    {
    }
    chunk::chunk(unsigned long long chunkId, string location)
    {
        this->chunkId = chunkId;
        this->location = location;
    }
    
    chunk::~chunk()
    {
    }

    unsigned long long chunk::getChunkId(){
        return this->chunkId;
    }

    string chunk::getLocation(){
        return this->location;
    }
} // namespace std