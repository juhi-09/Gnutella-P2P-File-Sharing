/********************************************************
* 				Author : Kaushik L V	 				*
*				Roll No: 20172138						*
*							 							*
*********************************************************/

#ifndef stringHandling
#include "CRS.h"
#endif

using namespace std;

// Function to handle search request
vector<vector<string> > searchRequest(CRSData crs, string alias, string msg) {
	vector<vector<string> > allSearchResults;

	bool clientPresent = false;
	for(auto cl : ClientList) {
		if(cl.first == alias) {
			clientPresent = true;
			break;			
		}
	}

	for(auto it : ClientList) {
		if(it.first == alias) {
			clientPresent = true;
			cout << alias << " is active" << endl;
			break;
		}
	}

	vector<string> searchResults;

	if(clientPresent == true) {
		string filename;
		msg = '('+ msg + ')' + ".*"; 
		regex regexp(msg, regex_constants::icase);
		smatch match;

		int noOfHits = 0;

		for(auto ml : masterList) {
			filename = get<0>(ml);
			// cout << "Inside For loop" << endl;
			// cout << filename << endl;
			// cout << "message to compare with " << msg << endl;
			std::map<string, string>::iterator it = ClientList.begin();			
						
			if(regex_search(filename, match, regexp)) {
				noOfHits++;
				//cout << get<0>(ml) << ":" << get<1>(ml) << ":" << get<2>(ml);
				searchResults.push_back(get<0>(ml));
				searchResults.push_back(":");
				searchResults.push_back(get<1>(ml));
				searchResults.push_back(":");
				searchResults.push_back(get<2>(ml));
				searchResults.push_back(":");
				it = ClientList.find(get<2>(ml));
				if(it != ClientList.end()) {
					//cout << ":" << (*it).second;
					searchResults.push_back((*it).second);
				}
				allSearchResults.push_back(searchResults);
				searchResults.clear();
			}
			
		}

		cout << "FOUND:" << noOfHits << endl;
		int k = 1;
		for (vector<vector<string> >::iterator i = allSearchResults.begin(); i != allSearchResults.end() && k != noOfHits+1; i++)
		{
			cout << "[" << k << "] ";
			for (vector<string>::iterator j = i->begin(); j != i->end(); j++)
			{
				cout << *j;
			}
			k++;
			cout << "\n";
		}	
	}

	return allSearchResults;

}

// Function to handle share request
string shareRequest(CRSData crs, string msg, int pos) {
	string result;
	string filename;	
	string path;

	// get command
	// cout << "Path" << path << endl;
	// cout << "Alias" << crs.client_alias << endl;
	
	for(auto ml : masterList) {
		if(get<1>(ml) == path) {  
			if(get<2>(ml) == crs.client_alias) {
				result = "FAILURE:ALREADY EXISTS";
				return result;
			}
		}
	}	

	int lastPos;
	int slashPos = msg.find_last_of("/");
	lastPos = msg.find_last_of('"');

	fstream repoFile;

	path = msg.substr(0, lastPos);		
	filename = msg.substr(slashPos+1, lastPos-slashPos-1);
	cout << path << endl;
	int file;
	if((file = open(path.c_str(), O_RDONLY)) < 0) {
		result = "FAILURE:FILE_NOT_FOUND";
		return result;
	}
	else {		
		auto record = make_tuple(filename, path, crs.client_alias);
		masterList.push_back(record);

		for(auto itr : masterList) {
		    cout << get<0>(itr) << " " << get<1>(itr) << " " << get<2>(itr) << "\n";
		}

		criticalSectionServer.lock();
		repoFile.open((crs.server_root + crs.main_repofile).c_str(), ios::out);

		for(auto itr : masterList) {
			repoFile.write(get<0>(itr).c_str(), strlen(get<0>(itr).c_str()));
			repoFile.write(":",1);
			repoFile.write(get<1>(itr).c_str(), strlen(get<1>(itr).c_str()));
			repoFile.write(":",1);
			repoFile.write(get<2>(itr).c_str(), strlen(get<2>(itr).c_str()));	
			repoFile.write("\n", 1);    	
		}
		repoFile.close();
		criticalSectionServer.unlock();

		result = "SUCCESS:FILE_SHARED";
	}
	return result;
}

// Function to handle get request
pair<string, string> getRequestDirect(string msg) {

	string alias;
	string path;
	string filename;

	msg = msg.substr(1);
	int secondQuote = msg.find_first_of('"');
	alias = msg.substr(0, secondQuote);

	int fourthQuote = msg.find_last_of('"');
	path = msg.substr(secondQuote+3, fourthQuote-(secondQuote+3));

	filename = msg.substr(fourthQuote+2);

	// cout << "\n" << alias << " " << path << " " << filename << endl;
	// cout.flush();

	int slashPos;
	size_t bSlashCount = count(alias.begin(), alias.end(), '\\');
	
	while(bSlashCount > 0) {
		slashPos = alias.find_first_of('\\');
		alias = alias.erase(slashPos, 1);
		bSlashCount--;
	}

	bSlashCount = count(path.begin(), path.end(), '\\');
	
	while(bSlashCount > 0) {
		slashPos = path.find_first_of('\\');
		path = path.erase(slashPos, 1);
		bSlashCount--;
	}

	bSlashCount = count(filename.begin(), filename.end(), '\\');
	
	while(bSlashCount > 0) {
		slashPos = filename.find_first_of('\\');
		filename = filename.erase(slashPos, 1);
		bSlashCount--;
	}

	string clientDetails;
	bool clientPresent = false;
	for(auto cl : ClientList) {
		if(cl.first == alias) {
			clientPresent = true;
			clientDetails = cl.second;	
		}
	}

	string result;
	if(clientPresent == true) {
		for(auto ml : masterList) {
			if(filename == get<0>(ml)) {
				result = "SUCCESS:" + get<1>(ml);
			}
		}
	}

	// cout << result << " " << clientDetails;
	//cout << "\n" << alias << " " << path << " " << filename << endl;

	pair<string, string> returnValues;
	returnValues = make_pair(result, clientDetails);

	return returnValues;

}

// Function to handle delete request
string deleteRequest(CRSData crs, string msg) {
	string result;
	int quotePos = msg.find_last_of('"');

	string path = msg.substr(0, quotePos);
	cout << "Entered path " << path << endl;
	cout.flush();
	bool isDeleted = false;
	auto itr = masterList.begin();
	while(itr != masterList.end()) {
		if(crs.client_alias == get<2>(*itr)) {
			if(path == get<1>(*itr)) {
				cout << "Found the record" << get<1>(*itr) << endl;
 				itr = masterList.erase(itr);
 				result = "SUCCESS:FILE_REMOVED";
 				isDeleted = true;
			}
			else {
				result = "FAILURE:FILE_NOT_FOUND";				
			}
		}
		itr++;
	}

	if(isDeleted) {
		// cout << "Printing contents of Repofile\n";
		// for(auto ml : masterList) {
		// 	cout << get<0>(ml) << " " << get<1>(ml) << " " << get<2>(ml) << endl;
		// }
		// cout << "Writing updated results to file\n";
		fstream repoFile;

		// cout << crs.server_root + crs.main_repofile << endl;
		criticalSectionServer.lock();
		repoFile.open((crs.server_root + crs.main_repofile).c_str(), ios::out);

		for(auto it : masterList) {
			repoFile.write(get<0>(it).c_str(), strlen(get<0>(it).c_str()));
			repoFile.write(":",1);
			repoFile.write(get<1>(it).c_str(), strlen(get<1>(it).c_str()));
			repoFile.write(":",1);
			repoFile.write(get<2>(it).c_str(), strlen(get<2>(it).c_str()));	
			repoFile.write("\n", 1);    	
		}
		repoFile.close();
		criticalSectionServer.unlock();
	}
	return result;
}

// Function to handle exec request
string executeRequest(string msg) {
	string alias;
	string command;
	int slashPos;
	size_t bSlashCount = count(msg.begin(), msg.end(), '\\');
	
	// Parse the string to get the ommand
	while(bSlashCount > 0) {
		slashPos = msg.find_first_of('\\');
		msg = msg.erase(slashPos, 1);
		bSlashCount--;
	}

	int quote;
	quote = msg.find_first_of('"');
	alias = msg.substr(0, quote);
	msg = msg.substr(quote+3);
	int lastQuote = msg.find_first_of('"');
	command = msg.substr(0, lastQuote);

	string clientDetails;
	bool clientPresent = false;
	for(auto cl : ClientList) {
		if(cl.first == alias) {
			clientPresent = true;
			clientDetails = cl.second;	
		}
	}
	command += ':';
	command += clientDetails;
	return command;	
}

// Function to handle all the commands that clients request.
void messageHandling(CRSData crs, string alias, char* message, int clSocket) {

	int pos = 0;
	string msg = message;

	int size = 1024;
	char response[size];

	// Dividing the first part of the message to see which command it is.
	pos = msg.find_first_of(" ");
	string command = msg.substr(0, pos);
	
	// Handling the share command
	if(command == "share") {
		string result;
		msg = msg.substr(pos+2);
		
		int lastQuote = msg.find_last_of('"');
		size_t noOfQuotes = count(msg.begin(), msg.end(), '"');
		string extra;
		extra = msg.substr(lastQuote+1);
		if(noOfQuotes > 2) {
			result = "FAILURE:INVALID_ARGUMENTS";
			result += '@'+'\0';
		}
		else {
			result = shareRequest(crs, msg, pos);
			cout << result << endl;
			result += '@'+'\0';
		}
		send(clSocket, result.c_str(), result.length(), 0);
	}

	// Handling the search command
	else if(command == "search") {
		vector<vector<string> > searchResults;
		msg = msg.substr(pos+1);
		string result;
		size_t noOfSpaces = count(msg.begin(), msg.end(), ' ');
		if(noOfSpaces > 0) {
			result = "FAILURE:INVALID_ARGUMENTS";
			result += '*'+'\0';
		}  
		searchResults = searchRequest(crs, alias, msg);
		int row = searchResults.size();
		int column = searchResults[0].size();

		// cout << "Sending the number of size of searchResults";
		string rows = to_string(row);
		string columns = to_string(column);
		// string dimensions = rows + ':' + columns;
		cout << rows << " " << columns;
		if(stoi(rows) == 0) {
			result = "FAILURE:FILE_NOT_FOUND";
			result += '*'+'\0';
		}
		else {
			result = "SUCCESS:FILE_FOUND";
			result += '*'+'\0';
		}
		bzero(response, size);
		send(clSocket, result.c_str(), result.length(), 0);
	}

	// Handling the get command
	else if(command == "get") {
		pair<string,string> result;
		msg = msg.substr(pos+1);
		result = getRequestDirect(msg);

		cout << result.first << " " << result.second;

		strcpy(response, result.first.c_str());
		strcat(response, "$");
		strcat(response, ":");
		strcat(response, result.second.c_str());

		send(clSocket, response, size, 0);
	}

	// Handling the del command
	else if(command == "del") {
		string result;
		size_t noOfQuotes = count(msg.begin(), msg.end(), '"');
		if(noOfQuotes > 2) {
			result = "FAILURE:INVALID ARGUMENTS";
		}
		else {
			msg = msg.substr(pos+2);
			result = deleteRequest(crs, msg);
			result += '^' + '\0';
		}

		send(clSocket, result.c_str(), result.length(), 0);
	}

	// Handling the get [1] command
	else if(command == "get []") {
	// Couldn't implement this 
	}	

	// Handling the exec command
	else if(command == "exec") {
		string execCommand;
		msg = msg.substr(pos+2);
		execCommand = executeRequest(msg);
	    execCommand += '&';
	    send(clSocket, execCommand.c_str(), execCommand.length(), 0);
	}

	// Handling the search command
	else {
		// string wrong = "INVALID_COMMAND";
		cout << "INVALID_COMMAND";			
	}

}
