# aarch64-none-linux-gnu-gcc or gcc

CC = gcc
ifdef CROSS_COMPILE
#aarch64-none-linux-gnu-
CC = $(CROSS_COMPILE)gcc
endif

CFLAGS= -Wall -Werror

all:aesdsocket	

default:aesdsocket

aesdsocket:aesdsocket.c	
	$(CC) -o aesdsocket aesdsocket.c $(CFLAGS)  

clean:
	rm -f *.raw