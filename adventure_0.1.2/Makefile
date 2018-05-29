CFLAGS = -Wall -mno-ms-bitfields -std=gnu99 -g

ifeq ($(OS),Windows_NT)
	LIBS = -lws2_32
	CLIENT = client.exe
	SERVER = server.exe
	ADMINTOOLS = admintools.exe
else
	LIBS = 
	CLIENT = client
	SERVER = server
	ADMINTOOLS = admintools
endif

# Adventure game by L Szabi 2013

test:
	echo $(OS)

all: $(CLIENT) $(SERVER) $(ADMINTOOLS)

$(CLIENT): client.o common.o md5.o console.o socket.o
	gcc $(CFLAGS) -o client $^ $(LIBS)

$(SERVER): server.o common.o database.o game.o console.o socket.o
	gcc $(CFLAGS) -o server $^ $(LIBS) -lm

$(ADMINTOOLS): admintools.o common.o database.o
	gcc $(CFLAGS) -o admintools $^

clean:
	rm -f *.o *.exe server client admintools

%.o : %.c
	gcc $(CFLAGS) -c -o $@ $<