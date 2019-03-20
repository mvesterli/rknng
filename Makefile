CC=g++
#CC=g++-5 #significantly faster code compared to g++-4

#DISABLEWARN = -Wno-unused-but-set-variable -Wno-unused-result
ARCHFLAGS =  -march=x86-64  -mno-avx
#CXXFLAGS +=  -O3 $(ARCHFLAGS)  -Wall -static -I.  -std=c++11
CXXFLAGS +=  -O3 $(ARCHFLAGS)  -Wall  -I.  -std=c++11

CFLAGS = -O3

DEPS = *.cpp *.h
OBJS=argtable3.o options.o
.PHONY:	all clean

PROGS= rknng annbenchmark_wrapper

all: rknng

#Should support compiling with g++, but there was a error message.
argtable3.o: contrib/argtable3.c
	gcc -c $(CFLAGS) contrib/argtable3.c
wrapper/protocol.o: wrapper/protocol.c
	gcc -c $(CFLAGS) -o $@ $^

options.o: options.c
	$(CC) -c $(CXXFLAGS) options.c

rknng: $(DEPS) $(OBJS)
	$(CC) $(CXXFLAGS) $(DISABLEWARN) knng.cpp       $(OBJS) -o rknng

clean:
	rm -f $(PROGS)
	find . -type f -name '*.o' -delete

apitest:
	g++ -O3 -c -std=c++11 -fPIC -o rknng_lib.o rknng_lib.cpp
	gcc -c options.c
	gcc -c apitest.c
	g++ -o apitest apitest.o rknng_lib.o options.o

annbenchmark_wrapper: wrapper/wrapper.o wrapper/protocol.o $(OBJS)
	$(CC) -o $@ $^
