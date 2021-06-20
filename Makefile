all:
	g++-8 -DMACOS -std=c++17 ikcp.c timer.cpp main.cpp udp_connection.cpp util.cpp -lpthread -o kcp-echo

.PHONY : all
