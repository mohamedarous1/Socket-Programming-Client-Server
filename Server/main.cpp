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

// time out handling variables.
struct timeval timeout{};
int nOfActiveConnections = 0;
pthread_mutex_t mutexHolder = PTHREAD_MUTEX_INITIALIZER;

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

void updateTimeOut() {
    timeout.tv_sec = (int)(20 / max(nOfActiveConnections, 1));
    timeout.tv_sec = max(10, (int)timeout.tv_sec);
}

void *connectionTransfer(void *arg){
    Arg arguments = *(Arg *) arg;
    while(true){

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(arguments.socketFD, &read_fds);
        int select_result = select(arguments.socketFD + 1, &read_fds, NULL, NULL, &timeout);

        if(select_result == 0) {
            printf("client socket with id : %d is closed\n" , arguments.socketFD);
            close(arguments.socketFD);
            pthread_mutex_lock(&mutexHolder);
            nOfActiveConnections--;
            updateTimeOut();
            pthread_mutex_unlock(&mutexHolder);
            return nullptr;
        }

        // handle receiving header
        char recievedMsg [BUFFER_SIZE];
        memset(recievedMsg, 0, sizeof(recievedMsg));
        ssize_t bytesReceived;
        bytesReceived = recv(arguments.socketFD, recievedMsg, BUFFER_SIZE, 0);

        string str(recievedMsg);
        /*
         * need some edit.
         * */
        if(str.empty()){
            printf("client socket with id : %d is closed\n" , arguments.socketFD);
            close(arguments.socketFD);
            pthread_mutex_lock(&mutexHolder);
            nOfActiveConnections--;
            updateTimeOut();
            pthread_mutex_unlock(&mutexHolder);
            return nullptr;
        }

        printf("got header request: \n%s\n", recievedMsg);
        if (bytesReceived < 0){
            printf("no header received in socketFD %d\n", arguments.socketFD);
            continue;
        }

        // parse given string into strings with delimiter " "
        regex delimiter(" ");
        sregex_token_iterator it(str.begin(), str.end(), delimiter, -1);
        sregex_token_iterator end;
        vector<string> header(it, end);


        if(header[0] == "GET") {

            // handle header
            ifstream file(header[1].substr(1, header[1].size() - 1));
            file.seekg(0, ios::end);
            pair<bool, string> contentStatus = fileContent(file);
            if(contentStatus.first){
                string sent_header = "HTTP/1.1 200 OK\r\n";
                sent_header.append("content-size: " + to_string(contentStatus.second.size()));
                sent_header.append(" \\r\\n");
                send(arguments.socketFD, sent_header.c_str(), sent_header.length(), 0);
            } else {
                string send_header = "HTTP/1.1 404 Not Found\\r\\n";
                send(arguments.socketFD, send_header.c_str(), send_header.length(), 0);
                continue;
            }
            // handle content
            send(arguments.socketFD, contentStatus.second.c_str(), contentStatus.second.length(), 0);
        }
        else if(header[0] == "POST"){
            int contentSize = stoi(header[8]);

            // handle got content.
            recievedMsg[bytesReceived] = '\0';
            ssize_t totalBytesReceived = 0;
            ofstream outputFile(header[1].substr(1, header[1].size() - 1), ios::binary);
            while ((bytesReceived = recv(arguments.socketFD, recievedMsg, sizeof(recievedMsg), 0)) > 0) {
                totalBytesReceived += bytesReceived;
                outputFile.write(recievedMsg, bytesReceived);
                if (totalBytesReceived >= contentSize) break;
            }

            // response to client message.
            string sent_header = "HTTP/1.1 200 OK\\r\\n";
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
        /*
         * initialization of timeout variables.
         * */
        timeout.tv_sec = 20;
        timeout.tv_usec = 0;
    } else {
        printf("Failed to create a socket.\n");
        exit(1);
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
    int listen_socket = listen(serverSocketFD , max_queue_size);
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
        } else {
            printf("Accept a request at socket ID: %d\n", connectionServerSockFD);
            pthread_mutex_lock(&mutexHolder);
            nOfActiveConnections++;
            updateTimeOut();
            pthread_mutex_unlock(&mutexHolder);
        }
        pthread_t thread;
        Arg arg;


        arg.socketFD = connectionServerSockFD;


        pthread_create(&thread  , NULL , connectionTransfer  ,  (void *) &arg);
    }
    close (serverSocketFD);


    return 0;
}
