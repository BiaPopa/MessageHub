CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

all: server subscriber

common.o: common.cpp

# Compileaza server.cpp
server: server.cpp common.o

# Compileaza subscriber.cpp
subscriber: subscriber.cpp common.o

clean:
	rm -rf server subscriber *.o *.dSYM
