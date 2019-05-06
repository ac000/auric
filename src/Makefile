CC=gcc
CFLAGS=-Wall -Wextra -Wno-deprecated-declarations -g -O2 -std=c99 -fPIC
LDFLAGS=-Wl,-z,defs,-z,relro,-z,now,--as-needed -pie
LIBS=`pkg-config --libs gtk+-3.0 glib-2.0 gmodule-2.0 cairo` -ltokyocabinet -lm
INCS=`pkg-config --cflags gtk+-3.0 glib-2.0 gmodule-2.0 cairo`

auric: auric.o callbacks.o interface.o
	$(CC) ${LDFLAGS} -o auric auric.o callbacks.o interface.o ${LIBS}

auric.o: common.h interface.h auric.h auric.c
	${CC} $(CFLAGS) -c auric.c ${INCS}

callbacks.o: common.h auric.h interface.h callbacks.c
	${CC} $(CFLAGS) -c callbacks.c ${INCS}

interface.o:  common.h interface.h interface.c
	${CC} $(CFLAGS) -c interface.c ${INCS}

clean:
	rm -f *.o auric
