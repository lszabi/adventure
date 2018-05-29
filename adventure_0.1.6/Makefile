CFLAGS=-Wall -g

# Adventure game by L Szabi 2013

win_cli: client.o common.o md5.o md5s.o
	gcc $(CFLAGS) -o client.exe $^ -lws2_32

win_serv: server.o common.o database.o game.o
	gcc $(CFLAGS) -o server.exe $^ -lws2_32 -lpthread

lin_cli: client.o  common.o md5.o md5s.o
	gcc $(CFLAGS) -o client $^ -ltermios

lin_serv: server.o common.o database.o game.o
	gcc $(CFLAGS) -o server $^ -lpthread

admintools: admintools.o common.o database.o
	gcc $(CFLAGS) -o admintools $^

clean:
	rm -f *.o *.exe *.out

%.o : %.c
	gcc $(CFLAGS) -c -o $@ $<

%.o : %.S
	gcc $(CFLAGS) -c -o $@ $<