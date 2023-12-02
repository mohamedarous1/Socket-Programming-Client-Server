#include <iostream>
#include <sys/socket.h>
#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<bits/stdc++.h>
#define DAFULT_PORT 80

using namespace std;
typedef long long ll;
typedef struct Arg Arg;
struct Arg {
    int socketFD;

};
bool CheckExistenceFile(string fileName){
    ifstream ifile;
    ifile.open(fileName);
    if(ifile)
        return true;
    else
        return false;
}


void *connectionTransfer(void *arg){
    Arg arguments = *(Arg *) arg;
    while(true){
        char recievedMsg [1024];

        memset(recievedMsg, 0, sizeof(recievedMsg));
        //strcpy(recievedMsg, "close");
        recv(arguments.socketFD, recievedMsg, 1024, 0);
        string str(recievedMsg);
        cout<< str <<" "<< str.size() ;
        std::regex delimiter(" ");
        std::sregex_token_iterator it(str.begin(), str.end(), delimiter, -1);
        std::sregex_token_iterator end;

        std::vector<std::string> header(it, end);
        if(header[0] ==  "close" || str.size() == 0  ){
            printf("bye bye id : %d\n" , arguments.socketFD );
            close(arguments.socketFD);
            break;
        }
        if(header[0] == "GET"){
            if(CheckExistenceFile(header[1])){
                ifstream ifile;
            }else{
                string send_header = "HTTP/1.1 404 Not Found\\r\\n";
                send(arguments.socketFD, send_header.c_str(), strlen(send_header.c_str()), 0);
            }
        }
        else if(header[0] == "POST"){

        }

        printf("The received data: %s from id : %d \n", recievedMsg , arguments.socketFD);

    }
    return 0;




}



int main(int argc, char* argv[]) {
    int port_number = DAFULT_PORT;
    if(argc == 2){
        port_number = stoi(argv[1]);
    }else if(argc > 2){
        printf("enter 2 argument only\n");
        exit(1);
    }
    //1. create a scoket for the server

    int serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFD != -1)
    {
        printf("Server Socket ID: %d\n", serverSocketFD);
    }
    else
    {
        printf("Failed to create a socket.\n"); exit(1);
    }
//2. bind this socket to a specific port number
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl (INADDR_ANY);
    serverAddress.sin_port = htons (port_number);
    if(bind(serverSocketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) ==  0){
        printf("Server is binded to port no. %d\n" , port_number);
    }
    else
    {
        printf("binfing Failed\n");
        close (serverSocketFD);
        exit(1);
    }
    //3. listen the connection
    int max_queue_size = 10;
    int listen_socket = listen(serverSocketFD , max_queue_size );
    if(listen_socket != 0){
        printf("failed\n");
    }
    vector<int> clientFD;

    // thread for time out should be here

    while(true){
        struct sockaddr_in connectedClientAddress;
        memset(&connectedClientAddress, 0, sizeof(connectedClientAddress));
        int clientAddrLength = 0;
        int connectionServerSockFD = accept(serverSocketFD, (struct sockaddr*)&connectedClientAddress ,reinterpret_cast<socklen_t *>(&clientAddrLength));
        if(connectionServerSockFD == -1)
        {
            printf("Failed to accept a connection request\n");
            exit(1);
        }
        else
        {
            printf("Accept a request at socket ID: %d\n", connectionServerSockFD);
        }
        pthread_t thread ;
        Arg  arg;
        arg.socketFD = connectionServerSockFD;
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        if (setsockopt (connectionServerSockFD, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                        sizeof timeout) < 0){
            printf("setsockopt failed\n");


        }


        if (setsockopt (connectionServerSockFD, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                        sizeof timeout) < 0){

            printf("setsockopt failed\n");

        }

        pthread_create(&thread  , NULL , connectionTransfer  ,  (void *) &arg);
    }
    close (serverSocketFD);


    return 0;
}
