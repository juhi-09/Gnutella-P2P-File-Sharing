/********************************************************
* 				Author : Kaushik L V	 				*
*				Roll No: 20172138						*
*							 							*
*********************************************************/

#ifndef CrsHead
#include "CRS.h"
#endif

mutex criticalSectionServer;

struct clientRequestData {
	int ret;
	int clientNumber;
	CRSData crs;
};

// Data structure to hold data of main_repo_file
vector<std::tuple<string, string, string> > masterList;

// Data structure to hold data of client_list_file
std::map<string, string> ClientList;

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

// Function which updates the crs log file
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
	
// Function which updates the client_list_file	
void updateClientData(clientRequestData cl, string clientFile, char* buffer) {

	fstream cfile;
	
	string buf = buffer;
	string alias;
	string remaining;
	std::pair<string, string> record;
	
	int pos = buf.find_first_of(':');
	alias = buf.substr(0, pos);
	remaining = buf.substr(pos+1);

	record = make_pair(alias, remaining);
	ClientList.insert(record);

	criticalSectionServer.lock();
	cfile.open((cl.crs.server_root + clientFile).c_str(), ios::app);
	
	for(auto cl:ClientList) {
		cfile.write(cl.first.c_str(), strlen(cl.first.c_str()));
		cfile.write(":",1);
		cfile.write(cl.second.c_str(), strlen(cl.second.c_str()));		
		cfile.write("\n", 1);
	}

	cfile.close();
	criticalSectionServer.unlock();
}

// Thread handler Function which is spawned for every incoming request
void *handleRequest(void *cl) {

	// Casting the void * object back to clientRequestData object.
	clientRequestData client = *((struct clientRequestData *)cl);

	bool isExit = false;

    // Structure to hold the data being transferred.
    int bufsize = 1024;
    char buffer[bufsize];

    // Structure for message passing 
    int messageSize = 1024;
    char message[messageSize];

    cout << "Thread Number " << pthread_self() << "\n";
    cout << "Got your details from Client : \n";
    
    // Receive the data from the client.
    recv(client.ret, buffer, bufsize, 0);
    cout << buffer << "\n";
    updateLogFile("repo.log",client.crs.client_alias, buffer);

    string buf = buffer;
    int pos = buf.find_first_of(':');
    client.crs.client_alias = buf.substr(0, pos);

    string file = client.crs.server_root + client.crs.client_list_file;
 
    // Function Call to update the details of client into file containing list of active clients 
    updateClientData(client, file, buffer);

    // Communication with the CRS
    do {    	
    	// Receiving message from the clients
    	cout << client.crs.client_alias << " : ";
    	memset(message, 0, messageSize);
        recv(client.ret, message, messageSize, 0);
        cout << message << " ";
        updateLogFile("repo.log",client.crs.client_alias, message);
        messageHandling(client.crs, client.crs.client_alias, message, client.ret);
        if (*buffer == '#') {
            isExit = true;
        }

        // Sending responses to client
        cout << "\nCRS: ";
        memset(message, 0, messageSize);
        cin.getline(message, messageSize);
        send(client.ret, message, messageSize, 0);
        if (*message == '#') {
            send(client.ret, message, messageSize, 0);
            isExit = true;
        }        
	} while (!isExit);

	// Exit from the thread to release all the resources acquired.
	close(client.ret);
	pthread_exit(EXIT_SUCCESS);
}

// Function which creates and handles the socket part
void socketHandling(CRSData crs) {

	// Identifier of the socket getting created.
	int client_socket;

    struct sockaddr_in CRS_addr;
    socklen_t size;

    // Creating the client socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Error checking for creation of socket
    if (client_socket < 0) {
        perror("\nError establishing socket...\n");
        exit(1);
    }

    cout << "Socket creation Successful" << endl;

    // Initializing the CRS address data structure with the CRS IP and port number.
    CRS_addr.sin_family = AF_INET;
    CRS_addr.sin_addr.s_addr = inet_addr(crs.server_ip.c_str());
    CRS_addr.sin_port = htons(crs.server_port);
    size = sizeof(CRS_addr);

    // Binding the client port to the CRS
    if ((bind(client_socket, (struct sockaddr*)&CRS_addr,sizeof(CRS_addr))) < 0) {
        perror("Error Binding connection in CRS\n");
        exit(1);
    }

    // Listening for clients
    listen(client_socket, 5);
	
	clientRequestData clReq;   

	int returnVal;
	clReq.crs = crs;	

    // Accept the connection from clients to CRS

    while(1) {    	
    	returnVal = accept(client_socket,(struct sockaddr *)&CRS_addr, &size);   	
    	
    	// Error checking for accept() call
	    if (returnVal < 0) 
	        perror("Error on accepting the connection");

	    //clReq.clientNumber = clientCount;
	    clReq.ret = returnVal;

	    // Function call to spawn a new thread and handle it for each client request.
	    //threadHandling(clReq); 

	    // Structures to maintain the thread	
		pthread_t requestThread;
		int returnCreate;

		// Create a new thread for CRS
		returnCreate = pthread_create(&requestThread, NULL, handleRequest, (void *)&clReq);

		// Error handling for thread creation
		if(returnCreate != 0) {
			perror("Thread creation failed\n");
			exit(EXIT_FAILURE);
		}
    } 
    close(client_socket);
}

int main(int argc, char* argv[]) {

	// Object to store all information about CRS.
	clientRequestData cl;

	if(argc != 6) {
		perror("Invalid number of arguments passed");
		exit(1);
	}

	// Taking inputs - 
	cl.crs.server_ip = argv[1];
	cl.crs.server_port = atoi(argv[2]);
	cl.crs.main_repofile = argv[3];
	cl.crs.client_list_file = argv[4];
	cl.crs.server_root = argv[5];

	// Initial Population of Client List

	fstream clientFile;

	string clLine;
	string file = cl.crs.server_root + cl.crs.client_list_file;
	clientFile.open(file.c_str(), ios::in);

	int firstColonCl, secondColonCl, thirdColonCl;
	
	string aliasClFile, clIP, clPort, dlPort;
	string remaining;

	ClientList.clear();
	while(getline(clientFile, clLine)) {
		firstColonCl = clLine.find_first_of(":");
		aliasClFile = clLine.substr(0, firstColonCl);
		clLine = clLine.substr(firstColonCl+1);
		secondColonCl = clLine.find_first_of(":");
		clIP = clLine.substr(0, secondColonCl);
		clLine = clLine.substr(secondColonCl+1);
		thirdColonCl = clLine.find_first_of(":");
		clPort = clLine.substr(0, thirdColonCl);
		dlPort = clLine.substr(thirdColonCl+1);
		remaining = clIP + ":" + clPort + ":" + dlPort;
		ClientList.insert(make_pair(aliasClFile, remaining));
	}

	// cout << "Initial population" << endl;
	// for(auto itr : ClientList) {
	//     cout << itr.first << " " << itr.second << "\n";
	// }

	clientFile.close();

	// Populating the data structure from the repo file

	fstream repo;

	string line;
	repo.open((cl.crs.server_root + cl.crs.main_repofile).c_str(), ios::in);

	int firstColon;
	int secondColon;

	string filename, path, aliasFile;

	masterList.clear();
	while(getline(repo, line)) {
		firstColon = line.find_first_of(":");
		filename = line.substr(0, firstColon);
		line = line.substr(firstColon+1);
		secondColon = line.find_first_of(":");
		path = line.substr(0, secondColon);
		aliasFile = line.substr(secondColon+1);
		masterList.push_back(make_tuple(filename, path, aliasFile));
	}

	// cout << "Initial population" << endl;
	// for(auto itr : masterList) {
	//     cout << get<0>(itr) << " " << get<1>(itr) << " " << get<2>(itr) << "\n";
	// }

	repo.close();

	// Function call to handle creation and maintenance of sockets.
	socketHandling(cl.crs);
	
	return 0;
}