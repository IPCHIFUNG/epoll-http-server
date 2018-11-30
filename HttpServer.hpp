#ifndef HTTPSERVER_HPP
#define HTTPSERVER_HPP

#include "HttpResponse.hpp"

#include <fstream>

#include <sys/epoll.h>

#define MAX_SOCK_SIZE		1024
#define FD_SIZE				1000
#define EPOLLEVENTS_SIZE	1000
#define BUFFER_SIZE			1024 * 1024

using namespace std;

class HttpServer
{
public:
	HttpServer();
	~HttpServer();
	
	bool init_server(unsigned short port);
	void start_serving();

private:
	void add_event(int fd, int state);
	void modify_event(int fd, int state);
	void delete_event(int fd);
	
private:
	int listen_fd = -1;
	int epfd = -1;
	struct epoll_event events[EPOLLEVENTS_SIZE];
	
	HttpResponse client_data[MAX_SOCK_SIZE];

};

#endif // HTTPSERVER_HPP