all:calClient calServer
calClient:cal_Client.cpp
	g++ -o calClient cal_Client.cpp -std=c++11
calServer:cal_Server.cpp
	g++ -o calServer cal_Server.cpp -std=c++11

.PHONY: clean
clean:
	rm -f calClient calServer log.txt
	