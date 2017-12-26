/********************************************************
* 				Author : Kaushik L V	 				*
*				Roll No: 20172138						*
*							 							*
*********************************************************/

#ifndef ClServer
#include "CRS.h"
#endif

// Function which sends the file to client on "get" request
void sendFiletoClient(int returnVal, string msg) {
	// Parsing the message
	
	int firstColon;
	firstColon = msg.find_first_of(':');
	cout << msg << endl;
	msg = msg.substr(firstColon+1);
	int secondColon;
	secondColon = msg.find_first_of(':');
	string path;
	path = msg.substr(0, secondColon);
	cout << "Inside Send Function\n";
	cout << "Sending the file path\n";
	cout << path.c_str() << "before send" << endl;
	send(returnVal, path.c_str(), strlen(path.c_str()), 0);
	cout << path << endl;

	// Sending the file
	int file;
	int sendCount = 0;
	ssize_t dataSent = 0, dataRead, fileSize = 0;
	int bufSize = 1024;
	char sendBuffer[bufSize];
	char errMsg[20] = "FILE_NOT_FOUND";

	cout << "Path in cstr" << path.c_str() << endl;
	// Opening the file path.c_str()
	if((file = open(path.c_str(), O_RDONLY)) < 0) {
		perror("\nFile could not be opened\n");
		if((dataSent = send(returnVal, errMsg, strlen(errMsg), 0)) < 0) {
			perror("\nUnable to send the data\n");
		}
	}
	else {
		cout << "Sending the file:\n";
		while((dataRead = read(file, sendBuffer, bufSize)) > 0) {
			if((dataSent = send(returnVal, sendBuffer, bufSize, 0)) < 0) {
				perror("Couldn't send the data\n");
			}
			fileSize += dataSent;
			sendCount++;
		}
		close(file);
	}
	cout << "No of Sends " << sendCount << endl;
	cout << "Sent file of size " << fileSize << endl;
	cout << "I am done sending bye!" << endl;
	close(returnVal);
}

// Thread handler function
void *startServer(void *cl) {

	ClientData client = *(struct ClientData *)cl;

	// Identifier of the socket getting created.
	int server_socket;

    struct sockaddr_in server_addr;
    socklen_t size;

    // Creating the client socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Error checking for creation of socket
    if (server_socket < 0) {
        perror("\nError establishing socket...\n");
        exit(1);
    }

    cout << "Client Server Socket creation Successful" << endl;

    // Initializing the CRS address data structure with the CRS IP and port number.
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(client.client_ip.c_str());
    server_addr.sin_port = htons(client.downloading_port);
    size = sizeof(server_addr);

    cout << "testing port number\n";
    cout << client.downloading_port << " " << client.client_ip << endl;

    // Binding the client port to the CRS
    if ((bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0) {
        perror("Error Binding connection in Client Server\n");
        exit(1);
    }

    // Listening for clients
    listen(server_socket, 5);

	//clientRequestData clReq;   
    //int returnVal[noOfClients];
	//int clientCount = 1;
	int returnVal;
	int messageSize = 1024;
	char message[messageSize];

	while(1) {    	
    	
    	// Accept the connection from other clients for download
	    returnVal = accept(server_socket,(struct sockaddr *)&server_addr, &size);

	    // Error checking for accept() call
	    if (returnVal < 0) 
	        perror("Error on accepting the connection");

	 //    int returnSer;
	 //    pthread_t newServerThread;
		
		// // Create a new thread to make client act as download server
		// returnSer = pthread_create(&newServerThread, NULL, startServer, (void *)&cl);

		// // Error handling for thread creation
		// if(returnSer != 0) {
		// 	perror("Server Thread Creation failed\n");
		// 	exit(EXIT_FAILURE);
	

	    bool isExit = false;
	    int hashPos;
	    string msg;

	    do {
	    	cout << "Client: ";	
	    	recv(returnVal, message, messageSize, 0); 
	    	cout << "\nAfter Receiving message inside loop" << endl;
	    	cout << message << endl; 
	    	msg = message;
	    	hashPos = msg.find_first_of('#');
	    	
	    	cout << hashPos;
	    	cout.flush();
	    	if(hashPos >= 0) {
		    	msg = msg.substr(0, msg.length()-1);		    	
		        cout << msg << " ";		
		        cout.flush();
		        isExit = true;       
		    }
		    else {
		    	isExit = true;
		    }
		} while (!isExit);

		sendFiletoClient(returnVal, msg);

        cout << "\nClient Server: ";
        memset(&message, 0, messageSize);          	
    }
}