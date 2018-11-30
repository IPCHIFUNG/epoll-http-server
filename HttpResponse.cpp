#include "HttpResponse.hpp"

#include <iostream>
#include <string.h>

using namespace std;

HttpResponse::HttpResponse()
{
}

HttpResponse::~HttpResponse()
{
}

bool HttpResponse::parse_request()
{
	string type = "GET";
	string path = "/";
	
	if (request[3] == ' ')
	{
		int i;
		for (i = 4; request[i] != '?' && request[i] != ' '; i++);
		path = request.substr(4, i - 4);
	}
	else
		type = "HEAD";
	
	buffer += "HTTP/1.1 200 OK\r\n";
	
	unsigned long file_size = 0;
	
	if (type == "GET")
	{
		string filename = path;
		if(path == "/")
			filename = "/index.html";
		
		string filepath = "html";
		filepath += filename;
		
		// cout << "Required file path = " << filepath << endl;
		
		in_file = new ifstream(filepath, ios::in | ios::binary);
		if(!in_file->is_open())
		{
			// cout << "Open file " << filepath << " failed!!!" << endl;
			in_file = new ifstream("html/404.html", ios::in | ios::binary);
			if(!in_file->is_open())
			{
				cerr << "Open file html/404.html failed!!!" << endl;
				return false;
			}
		}
		// Get the length of the file required.
		in_file->seekg(0, ios::end);
		file_size = in_file->tellg();
		in_file->seekg(0, ios::beg);
		
		// cout << "File size = " << file_size << endl;
		
		buffer += "Content-Length: ";
		buffer += to_string(file_size);
		buffer += "\r\n";
	}
	buffer += "\r\n";
	request = "";
	return true;
}
	
int HttpResponse::read_data(char *buf, int buf_size)
{
	// cout << "buffer = " << buffer << endl;
	if ((unsigned int)buf_size >= buffer.size() && buffer.size() != 0)
	{
		strcpy(buf, buffer.c_str());
		// cout << "buf = " << buf << endl;
		if (in_file == NULL)
			return (int)buffer.size();
		in_file->read(buf + buffer.size(), buf_size - (int)buffer.size());
		int data_read = in_file->gcount();
		in_file->seekg(-1 * data_read, ios::cur);
		return (int)buffer.size() + data_read;
	}
	else if (buffer.size() > 0)
	{
		string temp = buffer.substr(0, buf_size);
		strcpy(buf, temp.c_str());
		return buf_size;
	}
	else
	{
		if (in_file == NULL)
			return -1;
		// cout << "in_file != NULL" << endl;
		in_file->read(buf, buf_size);
		int data_read = in_file->gcount();
		in_file->seekg(-1 * data_read, ios::cur);
		return data_read;
	}
}

void HttpResponse::forward(int offset)
{
	if (buffer.size() > 0 && offset > (int)buffer.size())
	{
		offset -= (int)buffer.size();
		buffer = "";
		in_file->seekg(offset, ios::cur);
	}
	else if (buffer.size() > 0)
		buffer = buffer.substr(offset - 1, buffer.size() - offset);
	else
		in_file->seekg(offset, ios::cur);
}

bool HttpResponse::is_reading_finished()
{
	if (in_file != NULL)
		return in_file->peek() == EOF;
	return buffer.size() == 0;
}

void HttpResponse::close()
{
	if (in_file != NULL)
		in_file->close();
	in_file = NULL;
	buffer = "";
}