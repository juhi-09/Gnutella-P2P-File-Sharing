/********************************************************
*               Author : Kaushik L V                    *
*               Roll No: 20172138                       *
*                                                       *
*********************************************************/

#ifndef ClHead
#include "CRS.h"
#endif

mutex criticalSection;

// Function which gives the current time and date
string getTime() {
    string returnTime;
    struct tm* timeStruct;
    time_t currentTime;

    time(&currentTime);
    timeStruct = localtime(&currentTime);

    static char timeBuffer[20];
    strftime(timeBuffer, sizeof(timeBuffer), "%d:%m:%Y %H:%M:%S ", timeStruct);

    returnTime = timeBuffer;
    return returnTime;
}

// Function which updates the client log file
void updateLogFile(string filename, string alias, char* message) {
    string msg = message;
    string curTime = getTime();

    fstream logFile;
    logFile.open(filename.c_str(), ios::app);
    
    logFile.write(curTime.c_str(), curTime.length());
    logFile.write(" ", 1);  
    logFile.write(alias.c_str(), alias.length());
    logFile.write(" ", 1);
    logFile.write(msg.c_str(), msg.length());
    logFile.write("\n", 1); 

    logFile.close();
}

// Function called when client to execute RPC on another client machine
void connectToRpcServer(ClientData cl, string command, string targetIP, int clPort) {
    int rpcSocket;
    
    struct sockaddr_in target_addr;

    // Creating the client socket
    rpcSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Error checking for creation of socket
    if (rpcSocket < 0) {
        cout << "\nError establishing socket..." << endl;
        exit(1);
    }

    cout << "Client Port for RPC " << endl;
    cout << clPort << endl;
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(clPort);

    // Converts the IP to binary format and puts it into the server address structure.
    // We use #include <arpa/inet.h> to use this function.
    if (inet_pton(AF_INET, targetIP.c_str(), &target_addr.sin_addr) != 1) {
        perror("inet_pton in RPC failed");
        exit(1);
    }

    // Establishing the connection with CRS

    if (connect(rpcSocket,(struct sockaddr *)&target_addr, sizeof(target_addr)) == 0)
        cout << "Connection Established with RPC Server through Port: " << clPort << endl;
    else {
        cout << "Could not establish connection with Client Server\n"; 
        exit(1);       
    }

    cout << "Sending my Details\n";
    cout << "Hey there! I am a client\n";
    cout << "Execute this command for me\n";
    cout << command;
    cout.flush();

    command += '&';

    send(rpcSocket, command.c_str(), command.length(), 0);
    int receivedFile;
    int recvCount = 0;
    ssize_t dataReceived = 0, receivedFileSize = 0;
    int bufsize = 1024;
    char receiveBuffer[bufsize];
    string recvBuffer;
    string received;

    bzero(receiveBuffer, bufsize);

    criticalSection.lock();
    int nullPos;    
    if((receivedFile = open("execResults.txt", O_WRONLY|O_CREAT, 0644)) < 0) {
        perror("Couldn't create the file\n");
    }

    while((dataReceived = recv(rpcSocket, receiveBuffer, bufsize, 0)) > 0) {
        recvBuffer = receiveBuffer;
        nullPos = recvBuffer.find('\0');
        recvBuffer = recvBuffer.substr(0, nullPos);
        receivedFileSize += dataReceived;
        recvCount++;
        if(write(receivedFile, recvBuffer.c_str(), recvBuffer.length()) < 0) {
            perror("Unable to write to file\n");
        }                
    }

    close(receivedFile);
    criticalSection.unlock();
    cout << "Size of file received: " << receivedFileSize << " in " << recvCount << " recvs." << endl;
    cout.flush();
    criticalSection.lock();
    fstream showFile;
    showFile.open("execResults.txt", ios::in);
    string line;
    while(getline(showFile, line)) {
        line += '\n';
        cout << line; 
        cout.flush();  
    }
    showFile.close();

    criticalSection.unlock();
    close(rpcSocket);
    // pthread_exit(NULL);
}

// Function called when client wants to download a file from another client
void connectToClient(ClientData cl, string targetIP, int downloadPort, string downloadPath) {
    int downSocket;
    int bufsize = 1024;
    char buffer[bufsize];
    struct sockaddr_in target_addr;

    // Creating the client socket
    downSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Error checking for creation of socket
    if (downSocket < 0) {
        cout << "\nError establishing socket..." << endl;
        exit(1);
    }

    cout << downloadPort << endl;
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(downloadPort);

    // Converts the IP to binary format and puts it into the server address structure.
    // We use #include <arpa/inet.h> to use this function.
    if (inet_pton(AF_INET, targetIP.c_str(), &target_addr.sin_addr) != 1) {
        perror("inet_pton in Client Server failed");
        exit(1);
    }

    // Establishing the connection with CRS

    if (connect(downSocket,(struct sockaddr *)&target_addr, sizeof(target_addr)) == 0)
        cout << "Connection Established with Client Server through Port: " << downloadPort << endl;
    else {
        cout << "Could not establish connection with Client Server\n"; 
        exit(1);       
    }

    cout << "Sending my Details\n";
    cout << "Hey there! I am a client\n";

    string clPort = std::to_string(cl.client_port);
    string dlPort = std::to_string(downloadPort);

    strcpy(buffer, cl.client_alias.c_str());
    std::strcat(buffer, ":");
    std::strcat(buffer, downloadPath.c_str());
    std::strcat(buffer, ":");
    std::strcat(buffer, targetIP.c_str());
    std::strcat(buffer, ":");
    std::strcat(buffer, clPort.c_str());
    std::strcat(buffer, ":");
    std::strcat(buffer, dlPort.c_str());
    std::strcat(buffer, "#");

    cout << "Contents of Buffer" << endl;
    cout << buffer << endl;
    send(downSocket, buffer, bufsize, 0);
    memset(buffer, 0, bufsize);

    // Recieve the file
    bzero(buffer, bufsize);
    recv(downSocket, buffer, bufsize, 0);
    cout << "Got the file path\n";
    cout << buffer << endl;

    string buf = buffer;
    string filename;

    int lastSlash;
    lastSlash = buf.find_last_of('/');
    filename = buf.substr(lastSlash+1);
    cout << "Client Root" << cl.client_root;
    filename = cl.client_root + "/" + filename;
    cout << "Full Path with filename " << filename << endl;
    int receivedFile;
    int recvCount = 0;
    ssize_t dataReceived = 0, receivedFileSize = 0;
    char receiveBuffer[bufsize];
    string received;

    bzero(receiveBuffer, bufsize);
    // filename.c_str()
    criticalSection.lock();

    if((receivedFile = open(filename.c_str(), O_WRONLY|O_CREAT, 0644)) < 0) {
        perror("Couldn't create the file\n");
    }

    while((dataReceived = recv(downSocket, receiveBuffer, bufsize, 0)) > 0) {
        receivedFileSize += dataReceived;
        recvCount++;
        if(write(receivedFile, receiveBuffer, bufsize) < 0) {
            perror("Unable to write to file\n");
        }
                
    }

    cout << "Receive count " << recvCount << endl;
    cout.flush();

    close(receivedFile);
    criticalSection.unlock();

    cout << "Received file of size " << receivedFileSize << endl;
    cout.flush();

    close(downSocket);
}

// Thread handler function which for the client 
void *performOperation(void *client) {
	ClientData cl = *(struct ClientData *)client;

	int client_socket;	

    bool isExit = false;
    int bufsize = 1024;
    char buffer[bufsize];
    
    struct sockaddr_in CRS_addr;

    // Creating the client socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Error checking for creation of socket
    if (client_socket < 0) {
        cout << "\nError establishing socket..." << endl;
        close(client_socket);
        exit(1);
    }

    cout << "Client Socket creation Successful" << endl;

    // Specifies that we are using ipv4 and assigning the port number for CRS.
    CRS_addr.sin_family = AF_INET;
    CRS_addr.sin_port = htons(cl.crs_server_port);

    // Converts the IP to binary format and puts it into the server address structure.
    // We use #include <arpa/inet.h> to use this function.
    if (inet_pton(AF_INET, cl.server_ip.c_str(), &CRS_addr.sin_addr) != 1) {
        perror("inet_pton in CRS failed");
        exit(1);
    }

    // Establishing the connection with CRS
    //string connected;
    if (connect(client_socket,(struct sockaddr *)&CRS_addr, sizeof(CRS_addr)) == 0) {
        //connected = "Connection Established with CRS through Port: " + to_string(cl.crs_server_port);
        cout << "Connection Established with CRS through Port: " << cl.crs_server_port << endl;
        //updateLogFile("client.log", cl.client_alias, connected.c_str());
    }
    else {
        cout << "Could not establish connection with CRS\n";
        close(client_socket);
        exit(1);

    }

    cout << "Sending my Details";

    std::string clPort = std::to_string(cl.client_port);
    std::string dlPort = std::to_string(cl.downloading_port);

    strcpy(buffer, cl.client_alias.c_str());
    std::strcat(buffer, ":");
    std::strcat(buffer, cl.client_ip.c_str());
    std::strcat(buffer, ":");
    std::strcat(buffer, clPort.c_str());
    std::strcat(buffer, ":");
    std::strcat(buffer, dlPort.c_str());

    send(client_socket, buffer, bufsize, 0);
    memset(buffer, 0, bufsize);

    string response;
    size_t dollarPos;
    size_t firstColon;
    size_t colonPos;
    size_t secondColonPos;
    string targetIP;
    int downloadPort; 
    string downloadPath;

    do {
        cout << "\nClient: ";
        cin.getline(buffer, sizeof(buffer));
        string com;
        com = buffer;
        int space = com.find_first_of(" ");
        string request = com.substr(0, space);

        // if(request != "share" || request != "get" || request != "del" ||request != "exec" || request != "search") {
        //     cout << "INVALID_COMMAND";
        //     continue;
        // }
        
        if(request == "share") {
            int lastQuote = com.find_last_of('"');
            com = com.substr(space+2, (lastQuote-1)-(space+2));
            string path = cl.client_root + com;
            ifstream file;
            file.open(path);
            cout << "Path: " << path;
            if(!file.good()) {
                cout << "FAILURE:FILE_NOT_FOUND";
                continue;
            }
        }
        if(request == "get") {
            size_t noOfQuotes = count(com.begin(), com.end(), '"');
            string str = com.substr(space+1);
            if(noOfQuotes != 4) {
                cout << "FAILURE:INVALID_AGRUMENTS";
                continue;
            }            
        }        
        send(client_socket, buffer, bufsize, 0);
        if (*buffer == '#') {                
            send(client_socket, buffer, bufsize, 0);
            isExit = true;
        }
        
        cout << "\nCRS: ";
        bzero(buffer, bufsize);
        recv(client_socket, buffer, bufsize, 0);        
        response += buffer;
        // cout << "Before if's " << response;
        // cout.flush();
        size_t atPos = response.find_first_of('@');
        int countAt = count(response.begin(), response.end(), '@');
        int countDollar = count(response.begin(), response.end(), '$');
        int countAmp = count(response.begin(), response.end(), '&');
        int countCarat = count(response.begin(), response.end(), '^');
        int countStar = count(response.begin(), response.end(), '*');
        firstColon = response.find_first_of(':');
        dollarPos = response.find_first_of('$');
        // size_t ampPos = response.find_first_of('&');
        // size_t caratPos = response.find_first_of('^');
        downloadPath = response.substr(firstColon+1, (dollarPos-1)-firstColon);
        int i = 0;
        if(countAt > 0) {
            response = response.substr(0, atPos);
            int i = 0;
            while(buffer[i] != '@') {
                cout << buffer[i];
                i++;
            }
            cout << "\n";
        }
        else if(countDollar > 0) {
            i=0;
            while(response[i] != '$') {
                cout << response[i];
                i++;
            }
            response = response.substr(dollarPos+2);
            colonPos = response.find_first_of(':');
            targetIP = response.substr(0, colonPos);
            secondColonPos = response.find_last_of(':');
            downloadPort = stoi(response.substr(secondColonPos+1));
            cout << "\nParsed string\n";
            cout << targetIP << " " << downloadPort << " " << downloadPath;

            connectToClient(cl, targetIP, downloadPort, downloadPath);
        }
        else if(countCarat > 0) {
            int i = 0;
            while(buffer[i] != '^') {
                cout << buffer[i];
                i++;
            }
            cout << "\n";
        }
        else if(countStar > 0) {
            int i = 0;
            while(buffer[i] != '*') {
                cout << buffer[i];
                i++;
            }
            cout << "\n";
        }
        else if(countAmp > 0) {
            int i = 0;
            while(buffer[i] != '&') {
                response += buffer[i];
                i++;
            }
            int colonPos = response.find_first_of(':');                
            string cmd = response.substr(0, colonPos);
            
            response = response.substr(colonPos+1);
            colonPos = response.find_first_of(':');
            string targetIP = response.substr(0, colonPos);

            response = response.substr(colonPos+1);
            int secondColonPos = response.find_first_of(':');
            int clPort = stoi(response.substr(0, secondColonPos));
            cout.flush();

            connectToRpcServer(cl, cmd, targetIP, clPort);
        }
        else if (*buffer == '#') {                   
            isExit = true;
        }
        
        //updateLogFile("client.log", cl.client_alias, buffer);
    } while (!isExit);

    // Terminate the connection with CRS.
    close(client_socket);
    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
	if(argc != 8) {
		perror("Invalid number of arguments passed");
		exit(1);
	}

	// Taking inputs -
	ClientData cl;

	cl.client_alias = argv[1];
	cl.client_ip = argv[2];
	cl.client_port = atoi(argv[3]);
	cl.server_ip = argv[4];
	cl.crs_server_port = atoi(argv[5]);
	cl.downloading_port = atoi(argv[6]);
	cl.client_root = argv[7];	

	int returnVal;
	pthread_t newClientThread;
	void *joinThread;

    int returnSer;
    pthread_t newServerThread;
	
	// Create a new thread to make client act as download server
	returnSer = pthread_create(&newServerThread, NULL, startServer, (void *)&cl);

	// Error handling for thread creation
	if(returnSer != 0) {
		perror("Server Thread Creation failed\n");
		exit(EXIT_FAILURE);
	}

    pthread_t newRpcServerThread;

    // Create a new thread to make client act as download server
    returnSer = pthread_create(&newRpcServerThread, NULL, startRpcServer, (void *)&cl);

    // Error handling for thread creation
    if(returnSer != 0) {
        perror("Server Thread Creation failed\n");
        exit(EXIT_FAILURE);
    }

    // Create a new thread for the client
    returnVal = pthread_create(&newClientThread, NULL, performOperation, (void *)&cl);

    // Error handling for thread creation
    if(returnVal != 0) {
        perror("Client Thread creation failed\n");
        exit(EXIT_FAILURE);
    }   


    // Calling pthread_join that waits for completion of execution of the thread.
    returnVal = pthread_join(newClientThread, &joinThread);
    
    // Error handling for join function.
    if(returnVal != 0) {
        perror("Client Thread Join failed\n");
        exit(EXIT_FAILURE);
    }

	return 0;
}