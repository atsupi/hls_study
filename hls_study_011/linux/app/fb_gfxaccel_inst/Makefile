PROGRAM = fb_gfxaccel_inst
OBJS = fb_gfxaccel_inst.o
CC = arm-linux-gnueabihf-gcc
CFLAGS = -g -I./lib/include
LDFLAGS = -L./lib -lazplf_hal -lazplf_util

all : $(PROGRAM)

$(PROGRAM) : $(OBJS) ./lib/libazplf_hal.so ./lib/libazplf_util.so
	${CC} ${CFLAGS} $^ -o $@ ${LDFLAGS}

./lib/libazplf_hal.so:
	cd ${PWD}/lib/azplf_hal; make

./lib/libazplf_util.so:
	cd ${PWD}/lib/azplf_util; make

clean :
	rm -rfv *.o
	rm -rfv $(PROGRAM)

.PHONY : clean

# header file dependency

#iicps.o: iicps.h
