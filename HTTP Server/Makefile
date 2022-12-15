TARGETS = server

all: $(TARGETS)

server: server.cpp api_handler.cpp
	g++ -std=c++11 $^ -lpthread -g -o $@

clean::
	rm -fv $(TARGETS) *~
