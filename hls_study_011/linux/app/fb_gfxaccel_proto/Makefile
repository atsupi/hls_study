PROGRAM = fb_gfxaccel_proto
OBJS = fb_gfxaccel_proto.o vdma.o bitmap.o gfxaccel.o
CC = arm-linux-gnueabihf-gcc
CFLAGS = -g

all : $(PROGRAM)

$(PROGRAM) : $(OBJS)
	${CC} ${CFLAGS} $^ -o $@

clean :
	rm -rfv *.o
	rm -rfv $(PROGRAM)

.PHONY : clean

# header file dependency

#iicps.o: iicps.h
vdma.o: vdma.h
bitmap.o: bitmap.h
gfxaccel.o: gfxaccel.h
