LIBS = libazplf_util.so
OBJS = bitmap.o
CC = arm-linux-gnueabihf-gcc
CFLAGS = -g -shared -fPIC -I../include 

all : $(LIBS)
	cp ${LIBS} ../${LIBS}

$(LIBS) : $(OBJS)
	${CC} ${CFLAGS} $^ -o $@

clean :
	rm -rfv *.o
	rm -rfv $(LIBS)

.PHONY : clean

# header file dependency

#bitmap.o: bitmap.h
