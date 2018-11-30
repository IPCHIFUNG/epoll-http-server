#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <sys/epoll.h>

#include <regex>

#define MAX_SOCKET_SIZE		2048
#define FD_SIZE				1000
#define EPOLLEVENTS_SIZE	1000
#define BUFFER_SIZE			1024*128

using namespace std;

typedef struct Client
{
	string request = "";
	bool re = false;
	ifstream *in_file = NULL;
	unsigned long file_size = 0;
	unsigned long sent_size = 0;
} Client;

int main(int argc, char *argv[])
{
	cout << "begin" << endl;
	unsigned short port = 80;
	if (argc > 1)
		port = atoi(argv[1]);
	
	cout << "port " << port << endl;
	
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sock)
	{
		cerr << "Error: failed to create socket" << endl;
		return -1;
	}
	
	bool opt = false;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(&opt));
	
	sockaddr_in  saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = htonl(0);
	
	if (::bind(sock, (sockaddr*)&saddr, sizeof(saddr)) != 0)
	{
		cerr << "Error: failed to bind port (" << port << ")!" << endl;
		return -2;
	}
	
	cout << "Success in binding on port " << port << endl;
	
	if (listen(sock, 5) < 0)
	{
		cerr << "Error: failed to listen!!!" << endl;
		return -3;
	}
	
	Client client_data[MAX_SOCKET_SIZE];
	
	int epfd = epoll_create(FD_SIZE);
	struct epoll_event ev_listen;
	ev_listen.events = EPOLLIN;
	ev_listen.data.fd = sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev_listen);
	
	struct epoll_event events[EPOLLEVENTS_SIZE];
	
	for (;;)
	{
		int ret = epoll_wait(epfd, events, EPOLLEVENTS_SIZE, -1);
		cout << "========================================================================" << endl;
		cout << "DEBUG ==> ret = " << ret << endl;
		for (int i = 0; i < ret; i++)
		{
			cout << "===================================================" << endl;
			if (events[i].data.fd == sock)
			{
				cout << "events[i].data.fd == sock" << endl;
				sockaddr_in caddr;
				socklen_t len = sizeof(caddr);
				int client = accept(sock, (sockaddr*)&caddr, &len);
				if (client <= 0)
				{
					cerr << "==> Accept client failed!!!" << endl;
					continue;
				}
				cout << "Accepted a client = " << client << endl;
				client_data[client].request = "";
				client_data[client].re = false;
				struct epoll_event ev;
				ev.events = EPOLLIN;
				ev.data.fd = client;
				epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev);
			}
			else
			{
				int client = events[i].data.fd;
				if (events[i].events & EPOLLIN)
				{
					cout << "events[i].events & EPOLLIN (client = " << client << ")" << endl;
					
					if (!client_data[client].re)
					{
						char buf[BUFFER_SIZE] = { 0 };
						int revclen = recv(client, buf, BUFFER_SIZE, 0);
						if (revclen <= 0)
						{
							close(client);
							epoll_ctl(epfd, EPOLL_CTL_DEL, client, NULL);
							break;
						}
						client_data[client].request += buf;
						client_data[client].re = BUFFER_SIZE != revclen;
						if (!client_data[client].re)
							continue;
					}
					
					
					cout << "==========REVC=============\n" << client_data[client].request << "\n===========================" << endl;
					
					string pattern = "^([A-Z]+) /([a-zA-Z0-9]*([.][a-zA-Z]*)?)[?]?(.*) HTTP/1";
					regex r(pattern);
					smatch mas;
					regex_search(client_data[client].request, mas, r);
					if(mas.size() == 0)
					{
						cout << pattern << " failed!" << endl;
						continue;
					}
					string type = mas[1];
					string path = "/";
					path += mas[2];
					string filetype = mas[3];
					// string query = mas[4];
					
					if (type == "GET")
					{
						string filename = path;
						if(path == "/")
						{
							filename = "/index.html";
						}
						
						string filepath = "html";
						filepath += filename;
						
						cout << "Required file path = " << filepath << endl;
						
						client_data[client].in_file = new ifstream(filepath, ios::in | ios::binary);
						if(!client_data[client].in_file->is_open())
						{
							cout << "Open file " << filepath << " failed!!!" << endl;
							client_data[client].in_file = new ifstream("html/404.html", ios::in | ios::binary);
							if(!client_data[client].in_file->is_open())
							{
								cout << "Open file html/404.html failed!!!" << endl;
								continue;
							}
						}
						// Get the length of the file required.
						client_data[client].in_file->seekg(0, ios::end);
						client_data[client].file_size = client_data[client].in_file->tellg();
						client_data[client].in_file->seekg(0, ios::beg);
						
						cout << "File size = " << client_data[client].file_size << endl;
					}
					else if (type == "HEAD")
						client_data[client].file_size = 0;
					else
						continue;
					
					client_data[client].sent_size = 0;
					
					struct epoll_event ev;
					ev.events = EPOLLOUT;
					ev.data.fd = client;
					epoll_ctl(epfd, EPOLL_CTL_MOD, client, &ev);
				}
				else if (events[i].events & EPOLLOUT)
				{
					cout << "events[i].events & EPOLLOUT (client = " << client << ")" << endl;
					
					if (client_data[client].sent_size == 0)
					{
						stringstream ss;
						ss << "HTTP/1.1 200 OK\r\n";
						ss << "Content-Length: ";
						ss << client_data[client].file_size;
						ss << "\r\n\r\n";
						if ((unsigned int)send(client, ss.str().data(), ss.str().size(), MSG_NOSIGNAL) != ss.str().size())
							continue;
					}
					
					char buf[BUFFER_SIZE] = { 0 };
					client_data[client].in_file->read(buf, BUFFER_SIZE);
					int data_read = client_data[client].in_file->gcount();
					//client_data[client].in_file->seekg(-1 * data_read, ios::cur);
					cout << "data_read = " << data_read << endl;
					int data_sent = send(client, buf, data_read, MSG_NOSIGNAL);
					cout << "data_sent = " << data_sent << endl;
					if (data_sent == 0)
					{
						client_data[client].in_file->seekg(data_sent - data_read, ios::cur);
						continue;
					}
					if (data_sent < 0)
					{
						epoll_ctl(epfd, EPOLL_CTL_DEL, client, NULL);
						client_data[client].in_file->close();
						close(client);
						cout << "client " << client << " has been closed!!" << endl;
						continue;
					}
					//client_data[client].in_file->seekg(data_sent, ios::cur);
					client_data[client].in_file->seekg(data_sent - data_read, ios::cur);
					client_data[client].sent_size += data_sent;
					
					
					cout << client_data[client].sent_size << " data has been sent!" << endl;
					
					if (client_data[client].sent_size == client_data[client].file_size)
					{
						epoll_ctl(epfd, EPOLL_CTL_DEL, client, NULL);
						client_data[client].in_file->close();
						close(client);
						cout << "client " << client << " has been closed!!" << endl;
					}
				}
			}
			cout << "===================================================" << endl << endl;
		}
		cout << "========================================================================\n\n\n\n\n" << endl;
	}
	close(sock);
	return 0;
}