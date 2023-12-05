#include <iostream>
#include <sys/socket.h>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<bits/stdc++.h>
#define DAFULT_PORT 80
#define BUFFER_SIZE 1024
using namespace std;

void fast(){
    ios_base::sync_with_stdio(0);
    cin.tie(0);
    cout.tie(0);
}

string get_header( string  filePath){
    string buffer;
    buffer.append("GET /");
    buffer.append( filePath);
    buffer.append( " HTTP/1.1");
    buffer.append( "\r\n");
    return buffer;
}

vector<string> parser(string str) {
    // parse given string into strings with delimiter " "
    regex delimiter(" ");
    sregex_token_iterator it(str.begin(), str.end(), delimiter, -1);
    sregex_token_iterator end;
    vector<string> header(it, end);
    return header;
}

string post_header(string filePath , string hostName){
    string buffer;
    buffer.append("POST /");
    buffer.append( filePath);
    buffer.append( " HTTP/1.1 ");
    buffer.append( "Host: ");
    buffer.append( hostName);
    buffer.append( "\\r\\n ");
    buffer.append( "Connection: keep-alive\\r\\n ");
    return buffer;
}

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


int main(int argc, char* argv[]) {

    fast();

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
    string req;
    ifstream f;

    // get requests from file.
    vector<string> requests;
    f.open("request.txt");
    while(getline(f, req))
        requests.push_back(req);
    cout<<requests.size()<<endl;
    // for(auto &s : requests)
    for(int i = 0; i < requests.size(); ++i){
        auto &s = requests[i];
        vector<string> command = parser(s);

        if(command[0] ==  "client_get"){
            string send_header = get_header(command[1]);
            send(clientSockFD, send_header.c_str(), strlen(send_header.c_str()), 0);

            // handle receiving header
            char receivedMsg[BUFFER_SIZE];
            memset(receivedMsg, 0, sizeof(receivedMsg));
            ssize_t bytesReceived;
            bytesReceived = recv(clientSockFD, receivedMsg, BUFFER_SIZE,0);
            if (bytesReceived < 0){
                cout<<"recv() failed"<<endl;
                continue;
            }
            string str(receivedMsg);
            cout << "received header.." << str <<endl;
            vector<string> receivedHeader = parser(str);
            if(receivedHeader[1] != "200") continue;
            int contentSize = stoi(receivedHeader[3]);

            printf("before while loop\n");

            // handle got content.
            receivedMsg[bytesReceived] = '\0';
            ssize_t totalBytesReceived = 0;
            ofstream outputFile(command[1], ios::binary);
            while ((bytesReceived = recv(clientSockFD, receivedMsg, sizeof(receivedMsg), 0)) > 0) {
                totalBytesReceived += bytesReceived;
                outputFile.write(receivedMsg, bytesReceived);
                if (totalBytesReceived >= contentSize) break;
            }
            printf("%d total bytes received\n", (int)totalBytesReceived);
            printf("end recieve in get request");
        }
        else if(command[0] ==  "client_post"){

            // handle the header of post http request
            string send_header = post_header(command[1], command[2]);
            ifstream file(command[1]);
            file.seekg(0, ios::end);
            pair<bool, string> contentStatus = fileContent(file);

            //send(clientSockFD, send_header.c_str(), strlen(send_header.c_str()), 0);
            if(contentStatus.first){
                send_header.append("content-size: " + to_string(contentStatus.second.size()));
                send_header.append( " \\r\\n");
                printf("header in get in success %d: %s\n", i, send_header.c_str());
                send(clientSockFD, send_header.c_str(), send_header.length(), 0);
            } else {
                cout << "respond to request command "<< i << ": file not founded"  << '\n';
                continue;
            }
            // send the content of the file
            send(clientSockFD, contentStatus.second.c_str(), contentStatus.second.length(), 0);

            // receive response of post message.
            char response[BUFFER_SIZE];
            ssize_t bytesReceived = recv(clientSockFD, response, BUFFER_SIZE,0);
            if (bytesReceived < 0){
                cout<<"recv() failed"<<endl;
                continue;
            }
            printf("respond to HTTP request %d: %s", i, response);

        }

    }
    return 0;
}
