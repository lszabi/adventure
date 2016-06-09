AD_MAJ=0
AD_MIN=3
AD_REV=0

CFLAGS = -Wall -mno-ms-bitfields -std=gnu99 -g -fno-diagnostics-show-caret -fdiagnostics-color=always -DAD_MAJ=$(AD_MAJ) -DAD_MIN=$(AD_MIN) -DAD_REV=$(AD_REV)
DATE = $(shell date +'%Y%m%d')

ifeq ($(OS),Windows_NT)
	LIBS = -lws2_32
	CLIENT = client.exe
	SERVER = server.exe
else
	LIBS = 
	CLIENT = client
	SERVER = server
endif

# Adventure game by L Szabi 2013

test:
	echo $(OS)

backup:
	tar -cf adventure.tar *
	bzip2 adventure.tar
	mv adventure.tar.bz2 versions/adventure_$(DATE)_$(AD_MAJ).$(AD_MIN).$(AD_REV).tar.bz2

all: $(CLIENT) $(SERVER)

$(CLIENT): client.o util.o
	gcc $(CFLAGS) -o client $^ $(LIBS)

$(SERVER): server.o database.o util.o player.o
	gcc $(CFLAGS) -o server $^ $(LIBS) -lm

clean:
	rm -f *.o *.exe server client

%.o : %.c
	gcc $(CFLAGS) -c -o $@ $<