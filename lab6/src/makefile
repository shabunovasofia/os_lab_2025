CC = gcc
CFLAGS = -Wall
LIBS = -lpthread
LIB_NAME = libmultmodulo.a

all: client server

$(LIB_NAME): mult_modulo.o
	ar rcs $(LIB_NAME) mult_modulo.o

mult_modulo.o: mult_modulo.c mult_modulo.h
	$(CC) $(CFLAGS) -c mult_modulo.c

client: client.c $(LIB_NAME)
	$(CC) $(CFLAGS) -o client client.c -L. -lmultmodulo $(LIBS)

server: server.c $(LIB_NAME)
	$(CC) $(CFLAGS) -o server server.c -L. -lmultmodulo $(LIBS)

clean:
	rm -f client server *.o *.a

run_servers:
	./server --port 20001 --tnum 2 &
	./server --port 20002 --tnum 2 &
	./server --port 20003 --tnum 2 &

run_client:
	echo "127.0.0.1:20001" > servers.txt
	echo "127.0.0.1:20002" >> servers.txt
	echo "127.0.0.1:20003" >> servers.txt
	./client --k 50 --mod 997 --servers servers.txt

stop_servers:
	pkill server || true
