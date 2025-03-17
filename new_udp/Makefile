cc=g++

.PHONY: all 
all:udpClient udpServer

udpClient: udp_Client.cpp
	$(cc) -o $@ $^ -std=c++11 -lpthread
udpServer: udp_Server.cpp
	$(cc) -o $@ $^ -std=c++11

.PHONY: clean
clean:
	rm -f udpClient udpServer
