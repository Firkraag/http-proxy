CC = gcc

CSAPP_INC = ../include
CSAPP_SRC = ../lib

CFLAGS = -O1 -Wall -I $(CSAPP_INC) -I .
LDLIBS = -lpthread -lm

COUNTERARGS = -O1 -Wall -I $(CSAPP_INC)

# NOTE: we expect sharing.c to generate warnings
PROGS =	proxy

all: $(CSAPP_SRC)/csapp.o $(PROGS)
$(PROGS): $(CSAPP_SRC)/csapp.o 
$(CSAPP_SRC)/csapp.o: $(CSAPP_SRC)/csapp.c $(CSAPP_INC)/csapp.h

# The two programs that we use to illustrate synchronization
goodcnt: goodcnt.c 
	$(CC) $(COUNTERARGS) -o goodcnt goodcnt.c $(CSAPP_SRC)/csapp.o -lpthread
	$(CC) $(COUNTERARGS) -S goodcnt.c

badcnt: badcnt.c
	$(CC) $(COUNTERARGS) -o badcnt badcnt.c $(CSAPP_SRC)/csapp.o -lpthread
	$(CC) $(COUNTERARGS) -S badcnt.c

# Programs that need more than one .o file
echoserveri: echoserveri.o echo.o

clean:
	rm -f $(PROGS) *.o *~
