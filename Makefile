CC = g++
CC_FLAGS = -W -Wall -Werror -I./
EXEC = my_http
OBJS = main.o httpserver.o httpresponse.o

CC_FLAGS += -std=c++11

$(EXEC): $(OBJS)
	$(CC) $(CC_FLAGS) $(OBJS) -o $@

main.o: main.cpp
	$(CC) $(CC_FLAGS) -c -o $@ main.cpp
	
httpserver.o: HttpServer.cpp HttpServer.hpp
	$(CC) $(CC_FLAGS) -c -o $@ HttpServer.cpp
	
httpresponse.o: HttpResponse.cpp HttpResponse.hpp
	$(CC) $(CC_FLAGS) -c -o $@ HttpResponse.cpp

all: $(EXEC)

clean:
	rm -rf $(OBJS) $(EXEC)
