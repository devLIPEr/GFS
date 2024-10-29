#include <math.h>
#include <vector>

namespace std
{
    class util
    {
    private:
    public:
        static const int clientMasterPort  = 8080;
        static const int clientChunkPort   = 8081;
        static const int masterChunkPort   = 8082;
        static const int chunkChunkPort    = 8083;
        
        util(/* args */);
        ~util();

        static string number2IP(unsigned int ip);
        static unsigned int IP2number(string ip);
        static string fixIP(string ip);

        static string chunkId2String(unsigned long long id);

        template<typename T>
        static string serializeChunk(T c);
        template<typename T>
        static T deserializeChunk(string str);

        static vector<string> split(string str, char sep);

        static unsigned long long getChunkId();
    };
    
    util::util(/* args */)
    {
    }
    
    util::~util()
    {
    }
    
    string util::number2IP(unsigned int ip){
        string IP = "000.000.000.000";
        int pos = 12;
        while (ip != 0)
        {
            int x = ip & 255;
            for(int i = 0; i < 3; i++){
                int rest = x%((int)pow(10, i+1));
                IP[pos+2-i] = rest/(int)pow(10, i) + 48;
                x -= rest;
            }
            ip = ip >> 8;
            pos -= 4;
        }
        return IP;
    }
    
    unsigned int util::IP2number(string ip){
        int exp = 2, x = 0, k = 0;
        unsigned int r = 0;
        for(int i = 0; i < 16; i++){
            if(ip[i] != '.'){
                if(exp == -1){
                    r ^= x << 24-8*k;
                    k++;
                    exp = 2;
                    x = 0;
                }
                x += (ip[i]-48)*(int)pow(10, exp);
                exp--;
            }
        }
        return r;
    }

    string util::fixIP(string ip){
        string fixed = "";
        if(ip.length() < 15){
            int count = 0, last = 0;
            for(int i = 0; i < ip.length()+1; i++){
                if(ip[i] == '.' || ip[i] == '\0'){
                    if(count < 3){
                        for(int j = 0; j < 3-count; j++){
                            fixed += '0';
                        }
                    }
                    fixed += ip.substr(last, i-last);
                    fixed += '.';
                    count = 0;
                    last = i+1;
                }else{
                    count++;
                }
            }
            return fixed.substr(0, fixed.length()-1);
        }
        bool numberBeforeZeros = false;
        for(int i = 0; i < ip.length(); i++){
            if(ip[i] == '.'){
                fixed += '.';
                numberBeforeZeros = false;
            }else if(!numberBeforeZeros && ip[i] != '0'){
                numberBeforeZeros = true;
                fixed += ip[i];
            }else{
                fixed += ip[i];
            }
        }
        return fixed;
    }
    
    string util::chunkId2String(unsigned long long id){
        string r = "";
        unsigned long long mask = 18374686479671623680ULL; // 8 left most bits
        int shift = 56;
        for(int i = 0; i < 8; i++){
            r += char((id & mask) >> shift);
            mask = mask >> 8;
            shift -= 8;
        }
        return r;
    }

    template<typename T>
    string util::serializeChunk(T c){
        string r = chunkId2String(c.getChunkId()) + c.getLocation();
        return r;
    }

    template<typename T>
    T util::deserializeChunk(string str){
        unsigned long long id = 0;
        for(int i = 0; i < 8; i++){
            id ^= ((str[i] & 255) << (56-i*8));
        }
        T c = T(id, str.substr(8));
        return c;
    }
    
    vector<string> util::split(string str, char sep){
        vector<string> dirs;
        int p = 0;
        for(int i = 0; i < str.length(); i++){
            if(str[i] == sep){
                dirs.push_back(str.substr(p, i-p));
                p = i+1;
            }
        }
        return dirs;
    }

    unsigned long long util::getChunkId(){
        static unsigned long long chunkId = -1;
        chunkId++;
        return chunkId;
    }
} // namespace std
