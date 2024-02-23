CC=gcc
CFLAGS=-I. -g -Wall

client: client.c requests.c helpers.c buffer.c parson.c
	$(CC) -o client client.c requests.c helpers.c buffer.c parson.c $(CFLAGS)

run: client
	./client

clean:
	rm -f *.o client
