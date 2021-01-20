LIBS = libazplf_hal.so
OBJS = vdma.o gfxaccel.o lq070out.o
CC = arm-linux-gnueabihf-gcc
CFLAGS = -g  -shared -fPIC -I../include -v

all : $(LIBS)
	cp ${LIBS} ../${LIBS}

$(LIBS) : $(OBJS)
	${CC} ${CFLAGS} $^ -o $@

clean :
	rm -rfv *.o
	rm -rfv $(LIBS)

.PHONY : clean

# header file dependency

vdma.o: ../include/vdma.h
gfxaccel.o: ../include/gfxaccel.h
lq070out.o: ../include/lq070out.h
