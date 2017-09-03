TARGET = libmemq.a
all : $(TARGET)
	cp libmemq.a ../lib

LIBMEMCACHED_HOME =  ../../memcached

CPPFLAGS := $(CPPFLAGS) -I$(LIBMEMCACHED_HOME)/include 

LIBS := $(LIBS) -L$(LIBMEMCACHED_HOME)/lib/ -lmemcached -lcrypto -lrt

### Complie Rules. ###
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC
.cc.o:
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< -fPIC


OBJS2 = grouping.o memq.o sutils_common.o kfifo.o

libmemq.a: $(OBJS2)
	$(AR) -r $@ $(OBJS2)
	cp libmemq.a ../lib

clean:
	$(RM) $(TARGET) *.o
