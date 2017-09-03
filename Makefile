CC	= gcc

THIRD_PATH=./third

LIBMEMCACHED_HOME = $(THIRD_PATH)/memcached
COMMON_HOME	= $(THIRD_PATH)/common
SUTILS_CXX_HOME	= $(THIRD_PATH)/memq

LIBEVENT_HOME = $(THIRD_PATH)/libevent

SRC_DIR=src
BIN_DIR=bin


TARGET = agora

SRCC    = ${SRC_DIR}/tcp_server.c ${SRC_DIR}/config.c ${SRC_DIR}/daemon.c ${SRC_DIR}/mkdirs.c ${SRC_DIR}/squeue.c ${SRC_DIR}/slog.c ${SRC_DIR}/profile.c

BIN     = ${BIN_DIR}/agora

OBJS    = $(SRCC:.c=.o)



INCLUDE	= -I ./src -I $(LIBMEMCACHED_HOME)/include -I $(SUTILS_CXX_HOME)/include -I $(LIBEVENT_HOME)/include

LIBPATH	= -L lib -L $(LIBMEMCACHED_HOME)/lib/ -L $(SUTILS_CXX_HOME)/lib -lmemq  -L$(LIBEVENT_HOME)/lib/ -levent

LIBFILE	= -lmemcached -lpthread -ldl
#CFLAGS	= -ggdb -Wall $(INCLUDE) $(LIBPATH)
#CFLAGS	= -DDEBUG_TIME -O2 -ggdb -w $(INCLUDE) $(LIBPATH)
CFLAGS += -DMAIN


.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $@ $(LIBFILE)


all:$(BIN)


$(BIN): $(OBJS)	
	gcc -o $(BIN) $(OBJS) $(CFLAGS) $(LIBFILE)


clean:	
	-rm -f $(OBJS)	
	-rm -f $(BIN)	
