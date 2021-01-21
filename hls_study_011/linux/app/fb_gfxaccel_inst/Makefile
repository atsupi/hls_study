PROGRAM = fb_gfxaccel_inst
OBJS = fb_gfxaccel_inst.o
CC = arm-linux-gnueabihf-gcc
CFLAGS = -g -I./lib/include
LDFLAGS = -L./lib -L./ -lazplf_hal -lazplf_util

all : $(PROGRAM)

$(PROGRAM) : $(OBJS)
	${CC} ${CFLAGS} $^ -o $@ ${LDFLAGS}

clean :
	rm -rfv *.o
	rm -rfv $(PROGRAM)

.PHONY : clean

# header file dependency

#iicps.o: iicps.h
