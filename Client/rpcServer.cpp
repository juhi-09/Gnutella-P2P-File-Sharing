/********************************************************
*               Author : Kaushik L V                    *
*               Roll No: 20172138                       *
*                                                       *
*********************************************************/

#ifndef RPC
#include "CRS.h"
#endif

void *beginExecution(void *returnVal) {
    int rpc = *(int *)returnVal;
    int messageSize = 1024;
    char message[messageSize];
    bzero(message, messageSize);
    recv(rpc, message, messageSize, 0);
    cout << "Command Received" << endl;
    cout << message << endl;
    string str = message;
    string cmd;    
    int ampPos = str.find_first_of('&');
    cmd = str.substr(0, ampPos);
    cout << "Command is " << cmd;

    cmd += "> execResults.txt";

    system(cmd.c_str());

    // Sending the file
    int file;
    int sendCount = 0;
    ssize_t dataSent = 0, dataRead, fileSize = 0;
    int bufSize = 1024;
    char sendBuffer[bufSize];
    char errMsg[20] = "FILE_NOT_FOUND";

    if((file = open("execResults.txt", O_RDONLY)) < 0) {
        perror("\nFile could not be opened\n");
        if((dataSent = send(rpc, errMsg, strlen(errMsg), 0)) < 0) {
            perror("\nUnable to send the data\n");
        }
    }
    else {
        cout << "Sending the file:\n";
        while((dataRead = read(file, sendBuffer, bufSize)) > 0) {
            if((dataSent = send(rpc, sendBuffer, bufSize, 0)) < 0) {
                perror("Couldn't send the data\n");
            }
            fileSize += dataSent;
            sendCount++;
        }
        close(file);
    }
    close(rpc);     
}

// Thread handler function which starts the RPC server
void *startRpcServer(void *cl) {

	ClientData client = *(struct ClientData *)cl;

	// Identifier of the socket getting created.
	int rpc_server_socket;

    struct sockaddr_in rpc_server_addr;
    socklen_t size;

    // Creating the rpc server socket
    rpc_server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Error checking for creation of socket
    if (rpc_server_socket < 0) {
        perror("\nError establishing socket...\n");
        exit(1);
    }

    cout << "RPC Server Socket creation Successful" << endl;

    // Initializing the RPC address data structure with the client IP and client port number.
    rpc_server_addr.sin_family = AF_INET;
    rpc_server_addr.sin_addr.s_addr = inet_addr(client.client_ip.c_str());
    rpc_server_addr.sin_port = htons(client.client_port);
    size = sizeof(rpc_server_addr);

    cout << "testing port number\n";
    cout << client.client_port << " " << client.client_ip << endl;

    // Binding the client port to the RPC server
    if ((bind(rpc_server_socket, (struct sockaddr*)&rpc_server_addr, sizeof(rpc_server_addr))) < 0) {
        perror("Error Binding connection in RPC\n");
        exit(1);
    }

    // Listening for clients
    listen(rpc_server_socket, 5);

	int returnVal;
	int messageSize = 1024;
	char message[messageSize];

	// Accept the connection from other clients for download
    while(1) {
        returnVal = accept(rpc_server_socket,(struct sockaddr *)&rpc_server_addr, &size); 

        // Error checking for accept() call
        if (returnVal < 0) 
            perror("Error on accepting the connection");

        int rpcReturn;
        pthread_t newRpcThread;
        
        // Create a new thread to make client act as download server
        rpcReturn = pthread_create(&newRpcThread, NULL, beginExecution, (void *)&returnVal);

        // Error handling for thread creation
        if(rpcReturn != 0) {
            perror("Rpc Thread Creation failed\n");
            exit(EXIT_FAILURE); 
        }
    } 
}