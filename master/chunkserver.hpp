namespace std
{
    class chunkserver
    {
    private:
        string ip;
        unsigned long long freeSpace;
    public:
        chunkserver();
        chunkserver(string ip, unsigned long long freeSpace);
        ~chunkserver();

        bool operator<(chunkserver const& other) const { return this->freeSpace < other.freeSpace; }
        bool operator>(chunkserver const& other) const { return this->freeSpace > other.freeSpace; }

        unsigned long long getFreeSpace();
        void setFreeSpace(unsigned long long);
        string getIP();
    };

    chunkserver::chunkserver()
    {
    }
    chunkserver::chunkserver(string ip, unsigned long long freeSpace)
    {
        this->ip = ip;
        this->freeSpace = freeSpace;
    }

    chunkserver::~chunkserver()
    {
    }

    unsigned long long chunkserver::getFreeSpace(){
        return this->freeSpace;
    }

    void chunkserver::setFreeSpace(unsigned long long space){
        this->freeSpace = space;
    }

    string chunkserver::getIP(){
        return this->ip;
    }
} // namespace std
