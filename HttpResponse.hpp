#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <fstream>

using namespace std;

class HttpResponse
{
public:
	HttpResponse();
	~HttpResponse();

	string request = "";
	bool parse_request();
	
	int read_data(char *buf, int buf_size);
	void forward(int offset);
	
	bool is_reading_finished();
	
	void close();

private:
	ifstream *in_file = NULL;
	
	string buffer = "";
	
};

#endif // HTTPRESPONSE_HPP