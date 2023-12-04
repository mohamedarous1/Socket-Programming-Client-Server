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

pair<bool, string> getNeededFile(string &fileName){

    // Open the file ** assuming that the given path started by "/"

    ifstream fileStream;
//    if(fileName.find(".jpg")){
//
//       unsigned char * s = readImage(fileName.substr(1, fileName.size() - 1).c_str());
//        cout << string(reinterpret_cast<const char *>(s)) << endl;
//        return {1 , string(reinterpret_cast<const char *>(s))};
//    }else{
    fileStream.open( fileName );


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
    int i=0;
    while(i < s.length())
    {
        write(socket, beginner + i,min(maxNBytes, (int)s.length() - i+1));
        i+=500;
    }
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
    vector<string> requests;
    f.open("request.txt");
    while(getline(f, req))
        requests.push_back(req);
    cout<<requests.size()<<endl;
    for(auto s : requests){
        vector<string> command = parser(s);

        if(command[0] ==  "client_get" ){
            string send_header = get_header(command[1]);
            send(clientSockFD, send_header.c_str(), strlen(send_header.c_str()), 0);

            // handle receiving header
            char receivedMsg[BUFFER_SIZE];
            memset(receivedMsg, 0, sizeof(receivedMsg));
            ssize_t numBytesRcvd = recv(clientSockFD, receivedMsg, BUFFER_SIZE,0);
            if (numBytesRcvd < 0){
                cout<<"recv() failed"<<endl;
            }

            string str(receivedMsg);
            cout << "received header.." << str <<endl;
            vector<string> receivedHeader = parser(str);
            //for(auto x: receivedHeader) { cout << x << "\n"; }
            //..
            if(receivedHeader[1] != "200") continue;
            int contentSize = stoi(receivedHeader[3]);
            cout<<contentSize<<endl;


            string tempcontent = "";

            while (true) {

                int maxNBytes = 1024;
                char receivedContent[BUFFER_SIZE];
                memset(receivedContent, 0, sizeof(receivedContent));
                if(tempcontent.size() == contentSize){
                    break;
                }
                ssize_t valRead = recv(clientSockFD, receivedContent, BUFFER_SIZE,0);

                if (valRead <= 0) {
                    cout << "File Completed";
                    break;
                }
                tempcontent.append(string(receivedContent));
                cout<<string(receivedContent).size()<<endl;
                //cout<<tempcontent<<endl<<endl<<endl;
                cout<<tempcontent.size()<<endl<<endl;
                if(tempcontent.size() == contentSize){
                    break;
                }
            }
            cout<<"hello"<<endl;
            ofstream f_stream(command[1]);
            f_stream.write(tempcontent.c_str(), tempcontent.length());


        }
        else if(command[0] ==  "client_post"){

            // handle the header of post http request
            string send_header = post_header(command[1] , command[2]);
            cout<<"hellooooo"<<endl;
            pair<bool, string> contentStatus = getNeededFile(command[1]);
            //send(clientSockFD, send_header.c_str(), strlen(send_header.c_str()), 0);
            if(contentStatus.first){

                send_header.append("content-size: " + to_string(contentStatus.second.size()));
                send_header.append( " \\r\\n");
                cout << "header in get in success " << send_header << '\n';
                write(clientSockFD, send_header.c_str(), strlen(send_header.c_str()) );
            } else {
                cout << "file not founded"  << '\n';
                continue;
            }
            // send the content of the file
            cout << contentStatus.second << '\n';
            sendChuncks(clientSockFD, contentStatus.second);
        }

    }
    return 0;
}

    // handle got content.
//            char receivedContent[contentSize];
//            memset(receivedContent, 0, sizeof(receivedContent));
////            cout << "before receive\n";
//            recv(clientSockFD, receivedContent, contentSize, 0);
////            cout << "after receive\n";
//            string content(receivedContent);
////            cout << "printed content-size: " << content.size() << '\n';
////            cout << "received content :\n" << content << '\n';
//            printf("%s", content.c_str());