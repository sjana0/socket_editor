#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <csignal>
#include <climits>
#include <limits>
#include <unistd.h>
#include <sys/shm.h>

#define SUCCESSFUL_CONNECTION "successful connection"

#define SUCCESSFUL_COMMAND "command successful"
#define WRONG_COMMAND "wrong command"
#define PORT 5000

using namespace std;

int shmid = shmget(IPC_PRIVATE, 2*sizeof(int), 0777|IPC_CREAT);

int *a;

void parseCommand(string s, string str[4])
{
	str[0] = ""; str[1] = ""; str[2] = ""; str[3] = "";
	int count = 0;
	int i = 0;
	int p = 0;

	for(p = 0; p < s.length(); p++)
	{
		if(s[p] == ' ') {
			str[count++] = s.substr(i, p-i);
			i = p+1;
		}
		if(count == 3)
			break;
	}
	str[count] = s.substr(i);
}

bool fileExists (string name) {
	struct stat buffer;   
	return (stat (name.c_str(), &buffer) == 0); 
}

bool is_number(string& s) {
    string::const_iterator it = s.begin();
    while (it != s.end() && (isdigit(*it) || (it == s.begin() && *it == '-'))) ++it;
    return !s.empty() && it == s.end();
}

bool recv_ack(int sock)
{
	char buffer[50];
	string s;
	bzero(buffer, 50);
	int valread = recv(sock, buffer, 50, 0);
	buffer[valread] = '\0';
	s = buffer;
	buffer[0] = '\0';
	bzero(buffer, 50);
	if(s.compare("acknowledged") == 0)
		return true;
	else
	{
		cout << s <<"\n";
		return false;
	}
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

bool check_command(string s)
{
	string str[4];
	if(s.rfind("/users", 0) == 0 || s.rfind("/upload", 0) == 0
	|| s.rfind("/download", 0) == 0 || s.rfind("/insert", 0) == 0
	|| s.rfind("/read", 0) == 0 || s.rfind("/delete", 0) == 0
	|| s.rfind("/files", 0) == 0 || s.rfind("/invite", 0) == 0
	)
	{
		parseCommand(s, str);
		if(s.compare("/users") == 0 || s.compare("/files") == 0)
			return true;
		else if(s.rfind("/upload", 0) == 0 && str[2].compare("") == 0 && str[3].compare("") == 0 && fileExists(str[1]))
		{
			return true;
		}
		else if(s.rfind("/download", 0) == 0 && str[2].compare("") == 0 && str[3].compare("") == 0)
		{
			return true;
		}
		else if(s.rfind("/invite", 0) == 0 && (str[3][0] == 'V' || str[3][0] == 'E'))
		{
			return true;
		}
		else if(s.rfind("/insert", 0) == 0 || s.rfind("/read", 0) == 0 || s.rfind("/delete", 0) == 0)
		{
			return true;
		}
		return false;
	}
	else if(s.compare("/exit") == 0)
		return true;
	else
		return false;
}

void send_file(string filename, int sock)
{
	// sleep(1);
	char buffer[1024];
	string s;
	bzero(buffer, 1024);
	strcpy(buffer, filename.c_str());
	buffer[filename.length()] = '\0';
	send(sock, buffer, filename.length(), 0);
	buffer[0] = '\0';
	bzero(buffer, 1024);

	// sleep(1);
	ifstream fi(filename);
	while(!fi.eof())
	{
		buffer[0] = '\0';
		bzero(buffer, 1024);
		recv_ack(sock);
		getline(fi, s);

		strcpy(buffer, s.c_str());
		buffer[s.length()] = '\0';
		send(sock, buffer, s.length(), 0);
	}
	
	buffer[0] = '\0';
	bzero(buffer, 1024);
	recv_ack(sock);
	s = "eof";

	strcpy(buffer, s.c_str());
	buffer[s.length()] = '\0';
	send(sock, buffer, s.length(), 0);
	buffer[0] = '\0';
	bzero(buffer, 1024);
	recv_ack(sock);
}

string recv_file(int sock)
{
	char buffer[1024];
	string filename, s, s1;

	// recieve filename
	bzero(buffer, 1024);
	recv(sock, buffer, 1024, 0);
	filename = buffer;
	buffer[0] = '\0';
	bzero(buffer, 1024);
	send_ack(sock);
	
	// recieve first line of the file
	bzero(buffer, 1024);
	recv(sock, buffer, 1024, 0);
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
		recv(sock, buffer, 1024, 0);
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

void recv_contents(int sock)
{
	char buffer[1024];
	string s;

	// recieve first line of the file
	bzero(buffer, 1024);
	recv(sock, buffer, 1024, 0);
	s = buffer;
	buffer[0] = '\0';
	bzero(buffer, 1024);
	send_ack(sock);
	if(s.compare("eof") == 0)
	{
		cout << "result is empty!!\n";
	}
	else
	{
		while(s.compare("eof") != 0)
		{
			cout << s << "\n";
			// start reading file
			bzero(buffer, 1024);
			recv(sock, buffer, 1024, 0);
			s = buffer;
			buffer[0] = '\0';
			bzero(buffer, 1024);
			send_ack(sock);
		}
	}
}

void recv_clients(int sock)
{
	char buffer[6];
	int m, valread;
	string s;

	bzero(buffer, 6);
	valread = recv(sock, buffer, 6, 0);
	buffer[valread] = '\0';
	s = buffer;
	m = stoi(s);
	buffer[0] = '\0';
	bzero(buffer, 6);
	send_ack(sock);
	
	for(int i = 0; i < m; i++)
	{
		bzero(buffer, 6);
		valread = recv(sock, buffer, 6, 0);
		buffer[valread] = '\0';
		s = buffer;
		buffer[0] = '\0';
		bzero(buffer, 6);
		send_ack(sock);
		cout << i+1 <<  ": " << s << "\n";
	}
}

int main()
{
	// socket variables
	int sock_fd, n;
	char buffer[1024];
	string msg, s;
	struct hostent* server;
	server = gethostbyname("localhost");

	// attach shared memory segments
	a = (int*)shmat(shmid, 0, 0);
	a[0] = 0;

	bool flg = false;

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		cout << "\n Socket creation error \n";
		return -1;
	}

	struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;

	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

	serv_addr.sin_port = htons(PORT);

	if (connect(sock_fd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
	{
		perror("ERROR connecting");
	}

	bzero(buffer, 1024);
	recv(sock_fd, buffer, 1024, 0);
	msg = buffer;
	buffer[0] = '\0';
	bzero(buffer, 1024);
	send_ack(sock_fd);
	cout << msg << "\n";

	if(msg.rfind(SUCCESSFUL_CONNECTION, 0) == 0)
	{
		s = "";
		while(s.compare("/exit") != 0)
		{
			pid_t p = fork();
			if(p)
			{
				cout << "Give command:\n";
				s = "";
				getline(cin, s);
				if(check_command(s) && a[0] == 0)
				{
					// cout << "ok\n";
					kill(p, SIGABRT);
					buffer[0] = '\0';
					bzero(buffer, 1024);
					strcpy(buffer, s.c_str());
					buffer[s.length()] = '\0';
					send(sock_fd, buffer, s.length(), 0);
					buffer[0] = '\0';
					bzero(buffer, 1024);

					if(!recv_ack(sock_fd)){}

					else if(s.rfind("/users", 0) == 0)
					{
						send_ack(sock_fd);
						recv_clients(sock_fd);
					}
					else if(s.rfind("/upload", 0) == 0)
					{
						// send_ack();
						cout << s.substr(s.find(" ", 7)+1) << "\n";
						send_file(s.substr(s.find(" ", 7) + 1), sock_fd);
						cout << SUCCESSFUL_COMMAND << "\n";
					}
					else if(s.rfind("/download", 0) == 0)
					{
						send_ack(sock_fd);
						recv_file(sock_fd);
						cout << SUCCESSFUL_COMMAND << "\n";
					}
					else if(s.rfind("/insert", 0) == 0)
					{
						cout << SUCCESSFUL_COMMAND << "\n";
					}
					else if(s.rfind("/read", 0) == 0)
					{
						send_ack(sock_fd);
						recv_contents(sock_fd);
					}
					else if(s.rfind("/delete", 0) == 0)
					{
						cout << SUCCESSFUL_COMMAND << "\n";
					}
					else if(s.rfind("/files", 0) == 0)
					{
						char buf[1024];
						bzero(buf, 1024);
						send_ack(sock_fd);
						int valread = recv(sock_fd, buf, 4, 0);
						buf[valread] = '\0';
						s = buf;
						bzero(buf, 1024);
						send_ack(sock_fd);
						int nofiles = stoi(s);
						if(nofiles == 0)
						{
							cout << "no files in the server\n";
						}
						else
						{
							while(nofiles--)
							{
								cout << "in order filename:, no. of lines:, owner:, colaborators:\n";
								recv_contents(sock_fd);
								cout << "\n";
							}
						}
					}
					else if(s.rfind("/invite", 0) == 0)
					{
						// cout << "ok here\n";
						cout << "invitation accepted\n";
					}
					else if(s.compare("/exit") == 0)
						break;
				}
				else if(a[0] == 1)
				{
					while(1)
					{
						bzero(buffer, 1024);
						if(s.compare("yes") == 0)
						{
							buffer[0] = '1';
							send(sock_fd, buffer, 1, 0);
							break;
						}
						else if(s.compare("no") == 0)
						{
							buffer[0] = '0';
							send(sock_fd, buffer, 1, 0);
							break;
						}
						else
						{
							cout << "Please give a proper answer yes/no\n";
							getline(cin, s);
						}
					}
					a[0] = 0;
				}
				else
				{
					cout << WRONG_COMMAND << "\n";
				}
			}
			else
			{
				bzero(buffer, 1024);
				int valread = recv(sock_fd, buffer, 1024, 0);
				if(valread)
				{
					a[0] = 1;
					s = buffer;
					cout << "hello: " << s << "\n";
				}
				break;
			}
			
		}
	}
	else
	{
		cout << msg << "\n";
	}
	shmdt(a);
	shmctl(shmid, IPC_RMID, NULL);
}