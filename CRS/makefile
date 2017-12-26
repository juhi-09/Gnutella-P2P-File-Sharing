#/********************************************************
#* 		Author : Kaushik L V 			 *
#*		Roll No: 20172138			 *
#*							 *
#*********************************************************/

CC=g++
CFLAGS=-std=c++11 -I -Wall -lpthread
DEPS = CRS.h
OBJ = CRS.o messageHandling.o

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server_20172138: $(OBJ) $(DEPS)
	g++ -o $@ $^ $(CFLAGS)
