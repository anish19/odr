CC = gcc

LIBS =  /home/courses/cse533/Stevens/unpv13e/libunp.a

FLAGS = -g -O2 -I/home/courses/cse533/Stevens/unpv13e/lib

all: client_aa19 server_aa19 prhwaddrs_aa19 odr_aa19


prhwaddrs_aa19: prhwaddrs_aa19.o get_hw_addrs_aa19.o
	${CC} -g -o prhwaddrs_aa19 prhwaddrs_aa19.o get_hw_addrs_aa19.o ${LIBS}

client_aa19: client_aa19.o 
	${CC} -g -o $@ client_aa19.o ${LIBS}

server_aa19: server_aa19.o
	${CC} -g -o $@ server_aa19.o ${LIBS}

odr_aa19: odr_aa19.o get_hw_addrs_aa19.o
	${CC} -g -o $@ odr_aa19.o get_hw_addrs_aa19.o ${LIBS}


get_hw_addrs_aa19.o: get_hw_addrs_aa19.c
	${CC} ${FLAGS} -c get_hw_addrs_aa19.c

prhwaddrs_aa19.o: prhwaddrs_aa19.c
	${CC} ${FLAGS} -c prhwaddrs_aa19.c

client_aa19.o: client_aa19.c common.h
	${CC} ${FLAGS} -c client_aa19.c

server_aa19.o: server_aa19.c
	${CC} ${FLAGS} -c server_aa19.c

odr_aa19.o: odr_aa19.c 
	${CC} ${FLAGS} -c odr_aa19.c 

clean:
	rm *.o client_aa19 server_aa19 odr_aa19 prhwaddrs_aa19
