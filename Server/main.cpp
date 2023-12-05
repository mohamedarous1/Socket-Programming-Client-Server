#include <iostream>
#include <sys/socket.h>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<bits/stdc++.h>
#define DEFAULT_PORT 80
#define BUFFER_SIZE 1024
using namespace std;
typedef long long ll;
typedef struct Arg Arg;
struct Arg {
    int socketFD;
};

struct timeval timeout{};
int nOfActiveConnections = 0;

pair<bool, string> fileContent(ifstream &file) {
    if(!file.is_open()) return {0, ""};
    ostringstream fileContentStream;
    file.seekg(0, ios::beg);
    // Read the entire file into the stream
    fileContentStream << file.rdbuf();
    // Get the content as a string
    string fileContent = fileContentStream.str();
    return {1, fileContent};
}

void *connectionTransfer(void *arg){
    Arg arguments = *(Arg *) arg;
    while(true){
        // handle receiving header
        char recievedMsg [BUFFER_SIZE];
        memset(recievedMsg, 0, sizeof(recievedMsg));
        ssize_t bytesReceived;
        bytesReceived = recv(arguments.socketFD, recievedMsg, BUFFER_SIZE, 0);
        string str(recievedMsg);
        cout << str <<" ";
        if (bytesReceived < 0){
            cout<<"recv() failed"<<endl;
            continue;
        }

        // parse given string into strings with delimiter " "
        regex delimiter(" ");
        sregex_token_iterator it(str.begin(), str.end(), delimiter, -1);
        sregex_token_iterator end;
        vector<string> header(it, end);
        for(auto &h: header) {cout << h << '\n';}

        if(header[0] == "close" || str.empty()){
            printf("bye bye id : %d\n" , arguments.socketFD );
            close(arguments.socketFD);

            nOfActiveConnections--;

        }
        if(header[0] == "GET") {

            // handle header
            ifstream file(header[1].substr(1, header[1].size() - 1));
            file.seekg(0, ios::end);
            pair<bool, string> contentStatus = fileContent(file);
            if(contentStatus.first){
                string sent_header = "HTTP/1.1 200 OK\r\n";
                sent_header.append("content-size: " + to_string(contentStatus.second.size()));
                sent_header.append(" \\r\\n");
                cout << "header in get in success " << sent_header << '\n';
                send(arguments.socketFD, sent_header.c_str(), sent_header.length(), 0);
            } else {
                string send_header = "HTTP/1.1 404 Not Found\\r\\n";
                cout << "header in error: " << send_header << '\n';
                send(arguments.socketFD, send_header.c_str(), send_header.length(), 0);
                continue;
            }
            // handle content
            send(arguments.socketFD, contentStatus.second.c_str(), contentStatus.second.length(), 0);
        }
        else if(header[0] == "POST"){
            int contentSize = stoi(header[8]);
            cout<<contentSize<<endl;

            // handle got content.
            recievedMsg[bytesReceived] = '\0';
            ssize_t totalBytesReceived = 0;
            ofstream outputFile(header[1].substr(1, header[1].size() - 1), ios::binary);
            while ((bytesReceived = recv(arguments.socketFD, recievedMsg, sizeof(recievedMsg), 0)) > 0) {
                totalBytesReceived += bytesReceived;
                outputFile.write(recievedMsg, bytesReceived);
                if (totalBytesReceived >= contentSize) break;
            }
            printf("%d total bytes received\n", (int)totalBytesReceived);
            printf("end recieve in post request");

            // response to client message.
            string sent_header = "HTTP/1.1 200 OK\r\n";
            send(arguments.socketFD, sent_header.c_str(), sent_header.length(), 0);
        }
    }

    // need to close something??
    close(arguments.socketFD);
    return nullptr;




}

int main(int argc, char* argv[]) {
    int port_number = DEFAULT_PORT;
    if(argc == 2){
        port_number = stoi(argv[1]);
    }else if(argc > 2){
        printf("enter 2 argument only\n");
        exit(1);
    }
    //1. create a socket for the server

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
    struct sockaddr_in serverAddress{};
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
        struct sockaddr_in connectedClientAddress{};
        memset(&connectedClientAddress, 0, sizeof(connectedClientAddress));
        int clientAddrLength = 0;
        int connectionServerSockFD = accept(serverSocketFD, (struct sockaddr*)&connectedClientAddress ,reinterpret_cast<socklen_t *>(&clientAddrLength));
        if(connectionServerSockFD == -1)
        {
            printf("Failed to accept a connection request\n");
            exit(1);
        }
        else {
            printf("Accept a request at socket ID: %d\n", connectionServerSockFD);
            nOfActiveConnections++;
        }
        pthread_t thread ;
        Arg  arg;
        arg.socketFD = connectionServerSockFD;
        timeout.tv_sec = 100 / nOfActiveConnections;
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


/*
 * get >> client req get file
 *     >> server send header res then file
 * */