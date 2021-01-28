PROGRAM = fb_gfxaccel_hal
OBJS = fb_gfxaccel_hal.o sprite.o font.o
CC = arm-linux-gnueabihf-gcc
CFLAGS = -g -I./lib/include
LDFLAGS = -lpthread -L./lib -lazplf_hal -lazplf_util

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
	rm -rfv ./lib/lib*.so

.PHONY : clean

# header file dependency

#iicps.o: iicps.h
sprite.o: sprite.h
font.o: font.h
