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
	rm -f graph.txt
	rm -f gg
	rm -f TLM1_writer
	rm -f TLM2_writer
	rm -f graph_TLM1
	rm -f graph_TLM2
	rm -f py_writer
	rm -f graph_TLM1.cpp
	rm -f graph_TLM2.cpp
	rm -f mapperInput.py
	rm -f input/*
	rm -f lib/python/__pycache__/*

gg: $(SRC_DIR)/gg.cpp
	$(CC) $< -std=c++11 -Wall -o $@
	./gg

writeTLM1: gg
	$(CC) $(SRC_DIR)/TLM1_SC_writer.cpp -std=c++11 -Wall -o TLM1_writer
	./TLM1_writer

writeTLM2: gg
	$(CC) $(SRC_DIR)/TLM2_SC_writer.cpp -std=c++11 -Wall -o TLM2_writer
	./TLM2_writer

testTLM1: gg writeTLM1
	$(CC) $(CCFLAGS) graph_TLM1.cpp -o graph_TLM1
	./graph_TLM1

testTLM2: gg writeTLM2
	$(CC) $(CCFLAGS) graph_TLM2.cpp -o graph_TLM2
	./graph_TLM2

mapper: gg
	$(CC) $(SRC_DIR)/py_writer.cpp -std=c++11 -Wall -o py_writer
	./py_writer
	python3 mapperInput.py