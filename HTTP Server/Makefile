TARGETS = server

all: $(TARGETS)

server: server.cpp api_handler.cpp
	g++ -std=c++11 $^ -lpthread -g -o $@

mail: mail_service.cpp
	g++ -std=c++11 $< -lpthread -g -o $@

loadbalancer: load_balancer.cpp
	g++ -std=c++11 $< -lpthread -g -o $@

clean::
	rm -fv $(TARGETS) *~
