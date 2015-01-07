CC=9c
LD=9l
CFLAGS=
LDFLAGS=-lcurl -ljansson

all:V:	redditfs

redditfs:	redditfs.o reddit_client.o
	$LD $LDFLAGS -o redditfs $prereq

redditfs.o reddit_client.o:	reddit_client.h

%.o:	%.c
	$CC	$CFLAGS -c $stem.c

clean:V:
	rm -f *.o redditfs
	