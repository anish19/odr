CC = gcc

LIBS =  /home/courses/cse533/Stevens/unpv13e/libunp.a

FLAGS = -g -O2 -I/home/courses/cse533/Stevens/unpv13e/lib

all: client server odr prhwaddrs


prhwaddrs: prhwaddrs.o get_hw_addrs.o
	${CC} -g -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS}

client: client.o 
	${CC} -g -o $@ client.o ${LIBS}

server: server.o
	${CC} -g -o $@ server.o ${LIBS}

odr: odr.o
	${CC} -g -o $@ odr.o ${LIBS}


get_hw_addrs.o: get_hw_addrs.c
	${CC} ${FLAGS} -c get_hw_addrs.c

prhwaddrs.o: prhwaddrs.c
	${CC} ${FLAGS} -c prhwaddrs.c

client.o: client.c
	${CC} ${FLAGS} -c client.c

server.o: server.c
	${CC} ${FLAGS} -c server.c

odr.o: odr.c 
	${CC} ${FLAGS} -c odr.c 

clean:
	rm *.o client server odr prhwaddrs
