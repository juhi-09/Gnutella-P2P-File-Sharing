#ifndef CRS

#include <iostream>
#include <bits/stdc++.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <pthread.h>
#include <fstream>
#include <fcntl.h>
#include <regex>
#include <time.h>

using namespace std;

// Data Structure to take all the command line arguments of Client
struct ClientData
{	
	string client_alias;
	string client_ip;
	int client_port;
	string server_ip;
	int crs_server_port;	
	int downloading_port;
	string client_root;
};

// Data Structure to take all the command line arguments of CRS
struct CRSData
{
	string server_ip;
	int server_port;
	string main_repofile;
	string client_list_file;
	string server_root;
	string client_alias;
};

// Data Structures to store in the file
extern vector<std::tuple<string, string, string> > masterList;
extern std::map<string, string> ClientList;

void messageHandling(CRSData crs, string alias, char* message, int clSocket);
void *startServer(void *cl);
void *startRpcServer(void *cl);

#endif