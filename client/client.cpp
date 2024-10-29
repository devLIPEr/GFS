#include <iostream>
#include <fstream>
#include <string>

#include "../common/chunk.hpp"
#include "../common/util.hpp"
#include "rpc/client.h"
#include "rpc/rpc_error.h"

using namespace std;

void fileSecretary(chunk c){
    cout << c.getLocation() << '\n';
    rpc::client client(c.getLocation(), util::clientChunkPort);

    string operation, file;
    cout << "Insira a operacao para fazer no arquivo (escrever, ler, sair): ";
    while(cin >> operation && operation != "sair"){
        cout << "Insira o arquivo local: ";
        cin >> file;

        if(operation == "escrever"){
            ifstream f(file);
            string text;
            while(getline(f, text)){
                try{
                    client.call("write", c.getChunkId(), text, -1);
                }catch(rpc::rpc_error &e){
                    std::cout << std::endl << e.what() << std::endl;
                    std::cout << "in function '" << e.get_function_name() << "': ";
                    using err_t = std::tuple<int, std::string>;
                    auto err = e.get_error().as<err_t>();
                    std::cout << "[error " << std::get<0>(err) << "]: " << std::get<1>(err)
                            << std::endl;
                }
            }
            f.close();
        }else if(operation == "ler"){
            int start, number;
            cout << "Insira o inicio e o tamanho da leitura: ";
            cin >> start >> number;

            ofstream f(file);
            try{
                f << client.call("read", c.getChunkId(), start, number).as<string>();
            }catch(rpc::rpc_error &e){
                std::cout << std::endl << e.what() << std::endl;
                std::cout << "in function '" << e.get_function_name() << "': ";
                using err_t = std::tuple<int, std::string>;
                auto err = e.get_error().as<err_t>();
                std::cout << "[error " << std::get<0>(err) << "]: " << std::get<1>(err)
                        << std::endl;
            }
            f.close();
        }
        cout << "Insira a operacao para fazer no arquivo (escrever, ler, sair): ";
    }
}

int main(){
    string ip;
    cout << "Insira o IP (X.X.X.X) do servidor mestre: ";
    cin >> ip;

    rpc::client masterClient(ip, util::clientMasterPort);

    string operation, path, file;
    int index;
    chunk c;
    cout << "Insira a operacao para o arquivo (criar, procurar, deletar, sair) ";
    while(cin >> operation && operation != "sair"){
        try{
            cout << "Insira o caminho do arquivo (sem espacos), nome do arquivo (sem espacos) e o indice da chunk: ";
            cin >> path >> file >> index;
            if(operation == "criar"){
                c = util::deserializeChunk<chunk>(masterClient.call("create", path, file).as<string>());
                if(c.getLocation() != ""){
                    fileSecretary(c);
                }
            }else if(operation == "procurar"){
                c = util::deserializeChunk<chunk>(masterClient.call("get", path, file, index).as<string>());
                if(c.getLocation() != ""){
                    fileSecretary(c);
                }
            }else if(operation == "deletar"){
                masterClient.call("remove", path, file, index);
            }
            cout << "Insira a operacao para o arquivo (criar, procurar, deletar, sair) ";
        }catch(const std::exception& e){
            std::cerr << e.what() << '\n';
        }
    }

    return 0;
}