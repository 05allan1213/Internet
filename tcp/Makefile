all:tcpclient tcpserver
tcpclient:tcp_Client.cpp
	g++ -o tcpclient tcp_Client.cpp -lpthread -std=c++11
tcpserver:tcp_Server.cpp
	g++ -o tcpserver tcp_Server.cpp -lpthread -std=c++11

.PHONY: clean
clean:
	rm -f tcpclient tcpserver log.txt
	