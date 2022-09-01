#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/in.h>

#include <fcntl.h>
#include <dirent.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <time.h>

using namespace std;

#define INVALID_LINE_NUMBER "ERROR: the given line number is invalid"
#define FILE_NOT_EXISTS "ERROR: the file doesn't exists"
#define NOT_OWNER "ERROR: only owner have the permission"
#define PERMISSION_DENIED(s) "ERROR: do not have the permission to " + s
#define FILE_EXISTS_ERROR "ERROR: file already exists"
#define FILE_RANGE_ERROR "ERROR: index is out of range of the file"
#define USER_EXISTS_ERROR "ERROR: user doesn't exists"
#define INVITATION_DECLINED(s) s + " declined invitation"
#define SERVER_BUSY "unsuccessful connection server is busy(MAX 5 client reached)"
#define SUCCESSFUL_CONNECTION "successful connection"

#define SERVER_DIR "server_files/"
#define MAX_CLIENTS 5

const int randID() {
	return 1 + (std::rand() % (99999));
}

bool is_number(string& s) {
    string::const_iterator it = s.begin();
    while (it != s.end() && (isdigit(*it) || (it == s.begin() && *it == '-'))) ++it;
    return !s.empty() && it == s.end();
}

string make_copy(string filename) {
	string cp_filename = string(SERVER_DIR) + "temp_" + filename;
	filename = SERVER_DIR + filename;
	int read_fd;
	int write_fd;
	struct stat stat_buf;
	off_t offset = 0;
	read_fd = open (filename.c_str(), O_RDONLY);
	fstat (read_fd, &stat_buf);
	write_fd = open (cp_filename.c_str(), O_WRONLY | O_CREAT, stat_buf.st_mode);
	sendfile (write_fd, read_fd, &offset, stat_buf.st_size);
	close(read_fd);
	close(write_fd);
	return cp_filename;
}

bool fileExists (string name) {
	name = string(SERVER_DIR) + name;
	struct stat buffer;   
	return (stat (name.c_str(), &buffer) == 0); 
}

int countLines (string name) {
	string s, permFilename = string(SERVER_DIR) + "perm_" + name;
	name = string(SERVER_DIR) + name;

	if(fileExists(permFilename))
	{
		ifstream fi(permFilename);
		getline(fi, s);
		getline(fi, s);
		fi.close();
		return stoi(s);
	}
	else
	{
		int j = 0;
		ifstream fi(name);
		while(!fi.eof())
		{
			getline(fi, s);
			j++;
		}
		fi.close();
		return j;
	}
}

bool dirExists(const char *path) {
    struct stat info;

    if(stat( path, &info ) != 0)
        return false;
    else if(info.st_mode & S_IFDIR)
        return true;
    else
        return false;
}

bool chckDir(string dir) {
	if(!dirExists(dir.c_str()))
	{
		int check = mkdir(dir.c_str(),0777);
		if(!check)
			return false;
		else
			return true;
	}
	return true;
}

void send_ack(int sock)
{
	char buffer[20];
	string s = "acknowledged";
	bzero(buffer, 20);
	strcpy(buffer, s.c_str());
	send(sock, buffer, s.length(), 0);
	buffer[0] = '\0';
	bzero(buffer, 20);
}

bool recv_ack(int sock)
{
	char buffer[20];
	string s;
	bzero(buffer, 20);
	int valread = recv(sock, buffer, 20, 0);
	buffer[valread] = '\0';
	s = buffer;
	buffer[0] = '\0';
	bzero(buffer, 20);
	if(s.compare("acknowledged") == 0)
		return true;
	else
		return false;
}

void send_file(string filename, string filenameSend, int sock)
{
	char buffer[1024];
	string s;
	bzero(buffer, 1024);
	strcpy(buffer, filename.c_str());
	// buffer[filenameSend.length()] = '\0';
	send(sock, buffer, filename.length(), 0);
	buffer[0] = '\0';
	bzero(buffer, 1024);

	ifstream fi(filenameSend);
	while(!fi.eof())
	{
		buffer[0] = '\0';
		bzero(buffer, 1024);
		recv_ack(sock);
		getline(fi, s);

		strcpy(buffer, s.c_str());
		// buffer[filenameSend.length()] = '\0';
		send(sock, buffer, s.length(), 0);
	}
	buffer[0] = '\0';
	bzero(buffer, 1024);
	recv_ack(sock);
	s = "eof";

	strcpy(buffer, s.c_str());
	// buffer[filenameSend.length()] = '\0';
	send(sock, buffer, s.length(), 0);
	
	buffer[0] = '\0';
	bzero(buffer, 1024);
	recv_ack(sock);
}

string recv_file(int sock)
{
	char buffer[1024];
	string filename, s, s1;

	int valread;

	// recieve filename
	bzero(buffer, 1024);
	valread = recv(sock, buffer, 1024, 0);
	buffer[valread] = '\0';
	filename = buffer;
	buffer[0] = '\0';
	bzero(buffer, 1024);
	send_ack(sock);

	// modify filename for server
	filename = SERVER_DIR + filename;

	// recieve first line of the file
	bzero(buffer, 1024);
	valread = recv(sock, buffer, 1024, 0);
	buffer[valread] = '\0';
	s = buffer;
	buffer[0] = '\0';
	bzero(buffer, 1024);
	send_ack(sock);

	ofstream fo(filename);
	while(s.compare("eof") != 0)
	{
		s1 = s;

		// start reading file
		bzero(buffer, 1024);
		valread = recv(sock, buffer, 1024, 0);
		buffer[valread] = '\0';
		s = buffer;
		buffer[0] = '\0';
		bzero(buffer, 1024);
		send_ack(sock);
		
		if(s.compare("eof") != 0)
		{
			fo << s1 << endl;
		}
		else
		{
			fo << s1;
		}
	}
	fo.close();
	return filename;

}

void send_contents(string filename, int sock)
{
	char buffer[1024];
	string s;

	ifstream fi(filename);
	while(!fi.eof())
	{
		getline(fi, s);

		strcpy(buffer, s.c_str());
		send(sock, buffer, s.length(), 0);

		buffer[0] = '\0';
		bzero(buffer, 1024);
		recv_ack(sock);
	}

	s = "eof";
	strcpy(buffer, s.c_str());
	// buffer[filenameSend.length()] = '\0';
	send(sock, buffer, s.length(), 0);
	
	buffer[0] = '\0';
	bzero(buffer, 1024);
	recv_ack(sock);
}

void send_clients(int sock, string* str, int n)
{
	char buffer[6];
	
	int m = 0;
	for(int i = 0; i < n; i++)
	{
		if(str[i].compare("") != 0)
		{
			m++;
		}
	}

	bzero(buffer, 6);
	strcpy(buffer, to_string(m).c_str());
	send(sock, buffer, to_string(m).length(), 0);
	bzero(buffer, 6);
	recv_ack(sock);

	for(int i = 0; i < n; i++)
	{
		if(str[i].compare("") != 0)
		{
			strcpy(buffer, str[i].c_str());
			send(sock, buffer, str[i].length(), 0);
			buffer[0] = '\0';
			bzero(buffer, 6);
			recv_ack(sock);
		}
	}
}

void send_err(string s, int sock)
{
	cout << s << "\n";
	char buffer[1024];
	bzero(buffer, 1024);
	strcpy(buffer, s.c_str());
	send(sock, buffer, s.length(), 0);
	buffer[0] = '\0';
	bzero(buffer, 1024);
}

bool check_permission(string filename, string id, char perm)
{
	string s, permFilename = string(SERVER_DIR) + "perm_" + filename;
	filename = string(SERVER_DIR) + filename;

	ifstream fi(permFilename);
	
	getline(fi, s);
	getline(fi, s);
	getline(fi, s);

	if(s.rfind(id, 0) == 0)
		return true;
	else if(perm == 'O')
		return false;

	else
	{
		while(!fi.eof())
		{
			getline(fi, s);
			if(s.rfind(id, 0) == 0 && (s[s.length() - 1] == perm || s[s.length() - 1] == 'E'))
				return true;
			else if(s.rfind(id, 0) == 0)
			{
				cout << "permission file content:" << s << "\n";
				return false;
			}
		}
	}
	
	return false;
}

void update_permission(string filename, string id = "\0", char perm = '\0')
{
	string permFilename;
	int lines;
	if(filename.rfind(SERVER_DIR, 0) == 0)
	{
		permFilename = "perm_" + filename.substr(13);
		lines = countLines(filename.substr(13));
	}
	else
	{
		permFilename = "perm_" + filename;
		lines = countLines(filename);
	}
	if(perm == '\0')
	{
		string s, temp = make_copy(permFilename);
		ifstream fi(temp);
		ofstream fo(SERVER_DIR + permFilename);
		getline(fi, s);
		fo << s << endl;
		getline(fi, s);
		s = to_string(lines);
		fo << s << endl;
		while(!fi.eof())
		{
			getline(fi, s);
			if(fi.eof())
				fo << s;
			else
				fo << s << endl;
		}
		fi.close();
		fo.close();
		remove(temp.c_str());
	}
	else
	{
		ofstream fo;
		fo.open(SERVER_DIR + permFilename, ios_base::app);
		fo << endl;
		fo << id + " " + perm;
		fo.close();
	}
}

void upload(string filename, string owner, int sock)
{
	string userFile;
	if(!fileExists(filename))
	{
		userFile = string(SERVER_DIR) + owner + ".txt";
		if(fileExists(userFile))
		{
			ofstream fout;
			fout.open(userFile, ios_base::app);
			fout << endl << filename;
		}
		else
		{
			ofstream fout(userFile);
			fout << filename;
		}

		send_ack(sock);
		recv_file(sock);

		int lines = countLines(filename);
		
		string permFilename = string(SERVER_DIR) + "perm_" + filename;
		ofstream fo(permFilename);

		fo << filename << endl;
		fo << lines << endl;
		fo << owner;
	}
	else
	{
		send_err(FILE_EXISTS_ERROR, sock);
	}

}

void download(string filename, string id, int sock)
{
	if(check_permission(filename, id, 'V'))
	{
		send_ack(sock);
		recv_ack(sock);
		send_file(filename, string(SERVER_DIR) + filename, sock);
	}
	else
	{
		send_err(PERMISSION_DENIED(string("view")), sock);
	}
}

// for insert delete read and invite
void parseCommand(string s, string str[3])
{
	str[0] = ""; str[1] = ""; str[2] = "";
	int count = 0;
	int i = 0;
	int p = 0;

	for(p = 0; p < s.length(); p++)
	{
		if(s[p] == ' ') {
			str[count++] = s.substr(i, p-i);
			i = p+1;
		}
		if(count == 2)
			break;
	}
	str[count] = s.substr(i);
}

// insert
void insert(string filename, string msg, int idx, string id, int sock)
{
	if(fileExists(filename))
	{
		if(check_permission(filename, id, 'E'))
		{
			if(idx < 0)
				idx = countLines(filename) + idx;
			string outfilename = SERVER_DIR + filename;
			string infilename = make_copy(filename);
			ifstream fi(infilename);
			ofstream fo(outfilename);

			int j = 0;
			bool flg = false;
			string s;
			while(!fi.eof())
			{
				if(j == idx)
				{
					int foindx;
					while((foindx = msg.find("\\n")) != string::npos)
					{
						cout << "foindx = " << foindx << "\n";
						fo << msg.substr(0, foindx);
						fo << endl;
						msg = msg.substr(foindx+2);

					}
					fo << msg;
					fo << endl;
					flg = true;
					j++;
				}
				else
				{
					getline(fi, s);
					if(fi.eof() && flg)
						fo << s;
					else
						fo << s << endl;
					j++;
				}
			}
			if(!flg)
			{
				int foindx;
				while((foindx = msg.find("\\n")) != string::npos)
				{
					cout << "foindx = " << foindx << "\n";
					fo << msg.substr(0, foindx);
					fo << endl;
					msg = msg.substr(foindx+2);
				}
				fo << msg;
			}
			fi.close();
			fo.close();
			remove(infilename.c_str());
			update_permission(filename);
			send_ack(sock);
		}
		else
		{
			send_err(PERMISSION_DENIED(string("edit")), sock);
		}
	}
	else
	{
		send_err(FILE_NOT_EXISTS, sock);
	}
}

void insert(string filename, string msg, string id, int sock)
{
	if(fileExists(filename))
	{
		if(check_permission(filename, id , 'E'))
		{
			filename = SERVER_DIR + filename;
			ofstream fo;
			fo.open(filename, ios_base::app);
			int foindx;
			while((foindx = msg.find("\\n")) != string::npos)
			{
				cout << "foindx = " << foindx << "\n";
				fo << endl;
				fo << msg.substr(0, foindx);
				msg = msg.substr(foindx+2);

			}
			fo << endl;
			fo << msg.substr(0, foindx);
			update_permission(filename);
			send_ack(sock);
		}
		else
		{
			send_err(PERMISSION_DENIED(string("edit")), sock);
		}
	}
	else
	{
		send_err(FILE_NOT_EXISTS, sock);
	}
}

// read
void readLines(string filename, string id, int sock)
{
	string s;
	char buffer[1024];
	if(fileExists(filename))
	{
		if(check_permission(filename, id , 'E'))
		{
			send_ack(sock);
			recv_ack(sock);

			ifstream fi(string(SERVER_DIR) + filename);
			while(!fi.eof())
			{
				getline(fi, s);
				bzero(buffer, 1024);
				strcpy(buffer, s.c_str());
				send(sock, buffer, s.length(), 0);
				buffer[0] = '\0';
				bzero(buffer, 1024);
				recv_ack(sock);
			}

			s = "eof";
			bzero(buffer, 1024);
			strcpy(buffer, s.c_str());
			send(sock, buffer, s.length(), 0);
			buffer[0] = '\0';
			bzero(buffer, 1024);
			recv_ack(sock);
		}
		else
		{
			send_err(PERMISSION_DENIED(string("view")), sock);
		}
	}
	else
	{
		send_err(FILE_NOT_EXISTS, sock);
	}
}

void readLines(string filename, int idx, string id, int sock)
{
	string s;
	int j = 0;
	char buffer[1024];
	if(fileExists(filename))
	{
		if(check_permission(filename, id , 'E'))
		{
			send_ack(sock);
			recv_ack(sock);

			if(idx < 0)
				idx = countLines(filename) + idx;

			ifstream fi(string(SERVER_DIR) + filename);
			j = 0;
			
			while(!fi.eof())
			{
				getline(fi, s);
				if(j == idx)
				{
					bzero(buffer, 1024);
					strcpy(buffer, s.c_str());
					send(sock, buffer, s.length(), 0);
					buffer[0] = '\0';
					bzero(buffer, 1024);
					recv_ack(sock);
				}
				j++;
			}
			s = "eof";
			bzero(buffer, 1024);
			strcpy(buffer, s.c_str());
			send(sock, buffer, s.length(), 0);
			buffer[0] = '\0';
			bzero(buffer, 1024);
			recv_ack(sock);
		}
		else
		{
			send_err(PERMISSION_DENIED(string("view")), sock);
		}
	}
	else
	{
		send_err(FILE_NOT_EXISTS, sock);
	}
}

void readLines(string filename, int bdx, int edx, string id, int sock)
{
	string s;
	int j = 0;
	char buffer[1024];
	if(fileExists(filename))
	{
		if(check_permission(filename, id , 'E'))
		{
			send_ack(sock);
			recv_ack(sock);

			int lines = countLines(filename);

			if(bdx < 0 && edx < 0)
			{
				bdx = bdx + lines;
				edx = edx + lines;
			}

			ifstream fi(string(SERVER_DIR) + filename);
			j = 0;
			
			while(!fi.eof())
			{
				getline(fi, s);
				if(j >= bdx && j <= edx)
				{
					bzero(buffer, 1024);
					strcpy(buffer, s.c_str());
					send(sock, buffer, s.length(), 0);
					buffer[0] = '\0';
					bzero(buffer, 1024);
					recv_ack(sock);
				}
				j++;
			}
			s = "eof";
			bzero(buffer, 1024);
			strcpy(buffer, s.c_str());
			send(sock, buffer, s.length(), 0);
			buffer[0] = '\0';
			bzero(buffer, 1024);
			recv_ack(sock);
		}
		else
		{
			send_err(PERMISSION_DENIED(string("view")), sock);
		}
	}
	else
	{
		send_err(FILE_NOT_EXISTS, sock);
	}
}

// delete
void deleteLines(string filename, string id, int sock)
{
	if(fileExists(filename))
	{
		if(check_permission(filename, id , 'E'))
		{
			ofstream fo;
			filename = SERVER_DIR + filename;
			fo.open(filename, std::ofstream::out | std::ofstream::trunc);
			fo.close();
			update_permission(filename);
			send_ack(sock);
		}
		else
		{
			send_err(PERMISSION_DENIED(string("edit")), sock);
		}
	}
	else
	{
		send_err(FILE_NOT_EXISTS, sock);
	}
}
void deleteLines(string filename, int idx, string id, int sock)
{
	if(fileExists(filename))
	{
		if(check_permission(filename, id , 'E'))
		{
			if(idx < 0)
				idx = countLines(filename) + idx;
			string outfilename = SERVER_DIR + filename;
			string infilename = make_copy(filename);
			ifstream fi(infilename);
			ofstream fo(outfilename);

			int j = 0;
			bool flg = false, b = false;
			string s;
			while(!fi.eof())
			{
				if(j == idx)
				{
					getline(fi, s);
					flg = true;
				}
				else
				{
					getline(fi, s);
					if(!b)
					{
						b = true;
						fo << s;
					}
					else
						fo << endl << s;
				}
				j++;
			}

			fi.close();
			fo.close();
			remove(infilename.c_str());
			
			if(flg)
			{
				update_permission(filename);
				send_ack(sock);
			}

			else
				send_err(FILE_RANGE_ERROR, sock);
		}
		else
		{
			send_err(PERMISSION_DENIED(string("edit")), sock);
		}
	}
	else
	{
		send_err(FILE_NOT_EXISTS, sock);
	}
}
void deleteLines(string filename, int bdx, int edx, string id, int sock)
{
	if(fileExists(filename))
	{
		if(check_permission(filename, id , 'E'))
		{
			int bdx1, edx1, lines = countLines(filename);
			bool cxRange = false, b = false;

			if(bdx < 0 && edx < 0)
			{
				bdx = bdx + lines;
				edx = edx + lines;
			}
			if(bdx < 0 && edx > 0)
			{
				cxRange = true;
				bdx1 = bdx + lines;
				edx1 = lines - 1;
				bdx = 0;
			}
			string outfilename = SERVER_DIR + filename;
			string infilename = make_copy(filename);
			ifstream fi(infilename);
			ofstream fo(outfilename);

			int j = 0;
			bool flg = false;
			string s;
			while(!fi.eof())
			{
				if(!cxRange && j >= bdx && j <= edx)
				{
					getline(fi, s);
					flg = true;
				}
				else if(cxRange && ((j >= bdx && j <= edx) || (j >= bdx1 && j <= edx1)))
				{
					getline(fi, s);
					flg = true;
				}
				else
				{
					getline(fi, s);
					if(!b)
					{
						b = true;
						fo << s;
					}
					else
						fo << endl << s;
				}
				j++;
			}

			fi.close();
			fo.close();
			remove(infilename.c_str());
			
			if(flg)
			{
				update_permission(filename);
				send_ack(sock);
			}
			else
				send_err(FILE_RANGE_ERROR, sock);
		}
		else
		{
			send_err(PERMISSION_DENIED(string("edit")), sock);
		}
	}
	else
	{
		send_err(FILE_NOT_EXISTS, sock);
	}
}

// files
void files(int sock)
{
	int count = 0;
	char buffer[1024];
	string s;

	send_ack(sock);
	recv_ack(sock);
	
	string path = SERVER_DIR;
	struct dirent *entry;
	DIR *dir = opendir(path.c_str());
	
	if (dir == NULL) {
		return;
	}
	while ((entry = readdir(dir)) != NULL) {
		string s = entry->d_name;
		if(s.rfind("perm_", 0) == 0)
		{
			cout << s << endl;
			count++;
		}
	}

	bzero(buffer, 1024);
	s = to_string(count);
	strcpy(buffer, s.c_str());
	send(sock, buffer, s.length(), 0);
	buffer[0] = '\0';
	bzero(buffer, 1024);
	recv_ack(sock);

	closedir(dir);
	dir = opendir(path.c_str());

	if(count)
	{
		while ((entry = readdir(dir)) != NULL) {
			string s = entry->d_name;
			if(s.rfind("perm_", 0) == 0)
			{
				cout << s << endl;
				send_contents(SERVER_DIR + s, sock);
			}
		}
	}
	closedir(dir);

}

void exit_delete(string user)
{
	string userFile = string(SERVER_DIR) + user + ".txt";
	ifstream fi(userFile);
	while(!fi.eof())
	{
		string filename;
		getline(fi, filename);
		string permFile = string(SERVER_DIR) + "perm_" + filename;
		filename = SERVER_DIR + filename;
		remove(filename.c_str());
		remove(permFile.c_str());
	}
	fi.close();
	remove(userFile.c_str());
}

int main()
{
	// create directory for server side
	if(!chckDir(SERVER_DIR))
	{
		cout << "problem creating directory for files\n";
	}
	int PORT;
	cout << "Give the port:";
	cin >> PORT;

	// random number generator
	stringstream ss;
	string s;

	// rest of the code
	int opt = true;
	int master_socket , addrlen , new_socket , client_socket[MAX_CLIENTS], activity, i , valread , sd;
	int max_sd;
	struct sockaddr_in address;

	// client ids
	string client_ids[MAX_CLIENTS];
	 
	fd_set readfds;

	// int id = 50000;
	for (i = 0; i < MAX_CLIENTS; i++) 
	{
		client_socket[i] = 0;
	}

	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
 
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
 
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );
	 
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	cout << "Listener on port " <<  PORT << "\n";
	
	if (listen(master_socket, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	addrlen = sizeof(address);
	cout << "Waiting for connections ...";

	while(true)
	{
		FD_ZERO(&readfds);
 
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		for ( i = 0 ; i < MAX_CLIENTS ; i++) 
		{
			sd = client_socket[i];
			
			if(sd > 0)
				FD_SET( sd , &readfds);
			
			if(sd > max_sd)
				max_sd = sd;
		}

		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

		if ((activity < 0) && (errno!=EINTR)) 
		{
			cout << "select error\n";
		}

		if (FD_ISSET(master_socket, &readfds)) 
		{
			if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}
			char buffer[1024];
		 
			string msg = SERVER_BUSY;
			srand(time(0));
			for (i = 0; i < MAX_CLIENTS; i++) 
			{
				//if position is empty
				if( client_socket[i] == 0 )
				{
					// generate random number
					ss << std::setfill('0') << std::setw(5) << randID() << '\n';
					getline(ss, client_ids[i]);

					// clients
					client_socket[i] = new_socket;
					cout << "Adding to list of sockets as " << i << "\n";
					msg = "";
					msg = msg + SUCCESSFUL_CONNECTION + " your id:" + client_ids[i];
					break;
				}
			}

			buffer[0] = '\0';
			bzero(buffer, 1024);
			strcpy(buffer, msg.c_str());
			send(new_socket, buffer, msg.length(), 0);
			buffer[0] = '\0';
			bzero(buffer, 1024);
			recv_ack(new_socket);
		}

		for (i = 0; i < MAX_CLIENTS; i++) 
		{
			sd = client_socket[i];
			char buffer[1024];
			bzero(buffer, 1024);
			if (FD_ISSET( sd , &readfds)) 
			{
				//Check if it was for closing , and also read the incoming message
				valread = read( sd , buffer, 1024);
				buffer[valread] = '\0';
				string s = buffer;
				buffer[0] = '\0';
				bzero(buffer, 1024);
				cout << s << "\n";
				if (valread == 0)
				{
					getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
					cout << "Host disconnected , ip " << inet_ntoa(address.sin_addr) << ", port " << ntohs(address.sin_port) << "\n";
					 
					close( sd );
					client_socket[i] = 0;
					exit_delete(client_ids[i]);
					client_ids[i] = "";
				}
				 
				else
				{

					if(s.compare("/users") == 0)
					{
						send_ack(sd);
						recv_ack(sd);
						send_clients(sd, client_ids, MAX_CLIENTS);
					}
					else if(s.rfind("/upload", 0) == 0)
					{
						upload(s.substr(s.find(" ", 7)+1), client_ids[i], sd);
					}
					else if(s.rfind("/download", 0) == 0)
					{
						download(s.substr(s.find(" ", 8)+1), client_ids[i], sd);
					}
					else if(s.rfind("/insert", 0) == 0)
					{
						string str[3];
						parseCommand(s.substr(8), str);
						// cout << str[1] << "    " << str[2] << "\n";
						if(is_number(str[1]) && !str[2].compare("") == 0)
						{
							cout << stoi(str[1]) << "\n";
							str[2].pop_back();
							insert(str[0], str[2].substr(1), stoi(str[1]), client_ids[i], sd);
						}
						else
						{
							if(str[2].compare("") != 0)
								str[1] = str[1] + " " + str[2];
							str[1].pop_back();
							insert(str[0], str[1].substr(1), client_ids[i], sd);
						}
					}
					else if(s.rfind("/read", 0) == 0)
					{
						string str[3];
						parseCommand(s.substr(6), str);
						cout << str[0] << " " << str[1] << " " << str[2] << "\n";
						if(str[2].compare("") == 0)
						{
							if(str[1].compare("") == 0)
							{
								readLines(str[0], client_ids[i], sd);
							}
							else
							{
								cout << str[0] << " " << stoi(str[1]) << "\n";
								readLines(str[0], stoi(str[1]), client_ids[i], sd);
							}
						}
						else
						{
							readLines(str[0], stoi(str[1]), stoi(str[2]), client_ids[i], sd);
						}
					}
					else if(s.rfind("/delete", 0) == 0)
					{
						string str[3];
						parseCommand(s.substr(8), str);
						if(str[2].compare("") == 0)
						{
							if(str[1].compare("") == 0)
							{
								deleteLines(str[0], client_ids[i], sd);
							}
							else
							{
								cout << str[0] << " " << stoi(str[1]) << "\n";
								deleteLines(str[0], stoi(str[1]), client_ids[i], sd);
							}
						}
						else
						{
							deleteLines(str[0], stoi(str[1]), stoi(str[2]), client_ids[i], sd);
						}
					}
					else if(s.rfind("/files", 0) == 0)
					{
						files(sd);
					}
					else if(s.rfind("/invite", 0) == 0)
					{
						string str[3];
						parseCommand(s.substr(8), str);
						if(fileExists(str[0]))
						{
							if(check_permission(str[0], client_ids[i], 'O'))
							{
								int j;
								bool fg = false;
								for(j = 0; j < MAX_CLIENTS; j++)
								{
									if(client_ids[j].compare(str[1]) == 0)
									{
										fg = true;
										break;
									}
								}
								if(fg)
								{
									int sd1 = client_socket[j];
									string s2 = str[2][0] == 'E' ? "edit" : "read";
									string s1 = client_ids[i] + " invites you to " + s2 + " " + str[0];
									cout << s1 << "\n";
									bzero(buffer, 1024);
									strcpy(buffer, s1.c_str());
									send(sd1, buffer, s1.length(), 0);
									buffer[0] = '\0';
									bzero(buffer, 1024);
									recv(sd1, buffer, 1, 0);
									s1 = buffer;
									if(s1[0] == '0')
									{
										send_err(INVITATION_DECLINED(str[1]), sd);
									}
									else
									{
										update_permission(str[0], client_ids[j], str[2][0]);
										send_ack(sd);
									}

									cout << s1 << "\n";
								}
								else
								{
									send_err(USER_EXISTS_ERROR, sd);
								}
							}
							else
							{
								string s1 = "own";
								send_err(PERMISSION_DENIED(s1), sd);
							}
						}
						else
						{
							send_err(FILE_EXISTS_ERROR, sd);
						}
					}
					else
					{
						getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
						cout << "Host disconnected , ip " << inet_ntoa(address.sin_addr) << ", port " << ntohs(address.sin_port) << "\n";
						close(sd);
						client_socket[i] = 0;
						exit_delete(client_ids[i]);
						client_ids[i] = "";
					}
				}
			}
		}
	}
}