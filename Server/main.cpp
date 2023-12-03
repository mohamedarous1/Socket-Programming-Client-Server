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

using namespace std;
typedef long long ll;
typedef struct Arg Arg;
struct Arg {
    int socketFD;

};

pair<bool, string> getNeededFile(string &fileName){

    // Open the file ** assuming that the given path started by "/"
    ifstream fileStream;
    fileStream.open(fileName.substr(1, fileName.size() - 1));

    // Check if the file is open successfully
    if(!fileStream.is_open()) return {false, ""};

    // Read the entire content of the file into a string
    stringstream buffer;
    buffer << fileStream.rdbuf();
    string fileContent = buffer.str();

    // close the file
    fileStream.close();

    // return success
    return {true, fileContent};
}

void sendChuncks(int socket, string &s) {
    int maxNBytes = 500;
    const char *beginner = s.c_str();
    for (int i = 0; i < s.length(); i += maxNBytes) {
        send(socket, beginner + i,min(maxNBytes, (int)s.length() - i), 0);
    }
}

void *connectionTransfer(void *arg){
    Arg arguments = *(Arg *) arg;
    while(true){
        // handle receiving header
        char recievedMsg [1024];
        memset(recievedMsg, 0, sizeof(recievedMsg));
        recv(arguments.socketFD, recievedMsg, 1024, 0);
        string str(recievedMsg);
        cout << "received header.." << str.size() << str <<" ";

        // parse given string into strings with delimiter " "
        regex delimiter(" ");
        sregex_token_iterator it(str.begin(), str.end(), delimiter, -1);
        sregex_token_iterator end;
        vector<string> header(it, end);

        if(header[0] == "close" || str.empty()){
            printf("bye bye id : %d\n" , arguments.socketFD );
            close(arguments.socketFD);
            break;
        }
        if(header[0] == "GET") {
            pair<bool, string> contentStatus = getNeededFile(header[1]);

            // handle header
            if(contentStatus.first){
                string sent_header = "HTTP/1.1 200 OK\r\n";
                sent_header.append("content-size: " + to_string(contentStatus.second.size()));
                sent_header.append( " \r\n");
                cout << "header in get in success " << sent_header << '\n';
                 send(arguments.socketFD, sent_header.c_str(), strlen(sent_header.c_str()), 0);
            } else {
                string send_header = "HTTP/1.1 404 Not Found\r\n";
                send(arguments.socketFD, send_header.c_str(), strlen(send_header.c_str()), 0);
                continue;
            }
            cout << contentStatus.second << '\n';

            // handle content
            sendChuncks(arguments.socketFD, contentStatus.second);
        }
        else if(header[0] == "POST"){

        }

//        printf("The received data: %s from id : %d \n", recievedMsg , arguments.socketFD);

    }

    // need to close something??
    close(arguments.socketFD);
    return (void *)0;




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
        struct timeval timeout{};
        timeout.tv_sec = 100;
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