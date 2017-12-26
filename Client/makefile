#/********************************************************
#* 		Author : Kaushik L V 			 *
#*		Roll No: 20172138			 *
#*							 *
#*********************************************************/

CC=g++
CFLAGS=-std=c++11 -I -Wall  -lpthread
DEPS = CRS.h
OBJ = mainClient.o clientServer.o rpcServer.o

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client_20172138: $(OBJ)
	g++ -o $@ $^ $(CFLAGS)
