### Makefile for SGT-SYSTEMC Package Tools

### SystemC DEFINITIONS

CC = g++
SYSTEMC = $(SYSTEMC_HOME)
INCLUDE	= $(SYSTEMC)/include
LIBRARY = $(SYSTEMC)/lib-linux64
CCFLAGS = -Wall -I$(INCLUDE) \
			-L$(LIBRARY) -Xlinker -R -Xlinker $(LIBRARY) -lsystemc
SRC_DIR = src

### Tools

clean:
	rm -f *.txt
	rm -f *.out
	rm -f *.cpp
	rm -f *.py
	rm -f input/*
	rm -f lib/python/__pycache__/*

gg: $(SRC_DIR)/gg.cpp
	$(CC) $< -std=c++11 -Wall -o gg.out
	./gg.out

writeGCPP: gg
	$(CC) $(SRC_DIR)/writeGCPP.cpp -std=c++11 -Wall -o writeGCPP.out
	./writeGCPP.out

writeTLM1: gg
	$(CC) $(SRC_DIR)/writeTLM1.cpp -std=c++11 -Wall -o writeTLM1.out
	./writeTLM1.out

writeTLM2: gg
	$(CC) $(SRC_DIR)/writeTLM2.cpp -std=c++11 -Wall -o writeTLM2.out
	./writeTLM2.out

testGCPP: gg writeGCPP
	$(CC) graph.cpp -pthread -o gcpp.out
	./gcpp.out

testTLM1: gg writeTLM1
	$(CC) $(CCFLAGS) graph_TLM1.cpp -o graph_TLM1.out
	./graph_TLM1.out

testTLM2: gg writeTLM2
	$(CC) $(CCFLAGS) graph_TLM2.cpp -o graph_TLM2.out
	./graph_TLM2.out

mapper: gg
	$(CC) $(SRC_DIR)/writePY.cpp -std=c++11 -Wall -o writePY.out
	./writePY.out
	python3 mapperInput.py