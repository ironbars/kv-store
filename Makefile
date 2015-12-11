#ifeq ($(OS),Windows_NT)
    #SOURCES += winkv.c
#else
    #UNAME_S := $(shell uname -s)
    #ifeq ($(UNAME_S),Linux)
	#SOURCES += unixkv.c
	#CFLAGS += -pthread
    #endif
#endif

#CC= gcc
#CFLAGS+=-Wall -g -c
#LDFLAGS+=-pthread
#SOURCES+=kv.c kvserver.c kvclient.c
#OBJECTS=$(SOURCES:.c=.o)
#EXECUTABLES=kvserver kvclient

all: kv-cli kvserver kvsim

kvsim: kv.o kvsim.o unixkv.o
	gcc -pthread -Wall -g -o kvsim kv.o kvsim.o unixkv.o

kv-cli: kv.o kvclient.o unixkv.o
	gcc -pthread -Wall -g -o kv-cli kv.o kvclient.o unixkv.o

kvserver: kv.o kvserver.o unixkv.o
	gcc -pthread -Wall -g -o kvserver kv.o kvserver.o unixkv.o

kvsim.o: kvsim.c kv.h kvclient.h
	gcc -Wall -g -c kvsim.c

kvclient.o: kvclient.c kv.h kvclient.h
	gcc -Wall -g -c kvclient.c

kvserver.o: kvserver.c kv.h kvserver.h
	gcc -Wall -g -c kvserver.c

unixkv.o: unixkv.c unixkv.h kv.h
	gcc -Wall -g -c unixkv.c

kv.o: kv.c kv.h
	gcc -Wall -g -c kv.c

clean:
	rm *.o
