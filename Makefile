all:
	g++ -DMACOS -std=c++17 ikcp.c timer.cpp main.cpp udp_connection.cpp util.cpp -o kcp-echo

.PHONY : all
