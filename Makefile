CC = gcc# compiler
CFLAGS = -pthread -Wall -g# compile flags
LIBS = -lpthread -lrt# libs

all: mapper reducer

mapper: mapper.o
	$(CC) -o mapper mapper.o $(LIBS)

reducer: reducer.o
	$(CC) -o reducer reducer.o $(LIBS)

%.o:%.c
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm -f reducer mapper *.o *~
