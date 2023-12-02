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

string get_header( string  filePath){
    string buffer = "";
    buffer.append("GET /");
    buffer.append( filePath);
    buffer.append( " HTTP/1.1");
    buffer.append( "\r\n");
    return buffer;
}

//char * post_header(char filePath[] , char hostName[]){
//    char *buffer = (char *) malloc(1024);
//    strcpy(buffer, "POST /");
//    strcpy(buffer, filePath);
//    strcpy(buffer, "HTTP/1.1");
//    strcat(buffer, "Host: ");
//    strcat(buffer, hostName);
//    strcat(buffer, "\r\n");
//    strcat(buffer, "Connection: keep-alive\r\n");
//    return buffer;
//}




int main(int argc, char* argv[]) {
    int port_number = DAFULT_PORT;
    if(argc == 3){
        port_number = stoi(argv[2]);
    }else if(argc > 3){
        printf("enter 3 argument only\n");
        exit(1);
    }
    //1. create a socket for the client.
    int clientSockFD = socket (AF_INET, SOCK_STREAM, 0);
    if(clientSockFD == -1) {
        printf("Failed to create a socket.\n");
        exit(1);
    }

    //2. connect the client to a specific server
    printf("Client Socket ID: %d\n", clientSockFD);
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port_number);

    int connectionTest =
            connect(clientSockFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if(connectionTest != 0) {
        printf("Failed to make a connection.\n");
        exit(1);
    }
    string s;
    while(true){
        getline(std::cin, s);
        std::regex delimiter(" ");
        std::sregex_token_iterator it(s.begin(), s.end(), delimiter, -1);
        std::sregex_token_iterator end;
        std::vector<std::string> command(it, end);

        if(command[0] ==  "client_get" ){
            string send_header = get_header(command[1]);
            //printf("%s\n" , send_header);
            send(clientSockFD, send_header.c_str(), strlen(send_header.c_str()), 0);

        }

        //else if(strcmp(command[0] , "client_post" )==0){
//            char * temp ;
//            strcpy(temp , command[1]);
//            char * temp2 ;
//            strcpy(temp2 , command[2]);
//            char *send_header = post_header(temp , temp2);
//            send(clientSockFD, send_header, strlen(send_header), 0);
//        }
        //send(clientSockFD, s, strlen(s), 0);

    }
    return 0;
}
