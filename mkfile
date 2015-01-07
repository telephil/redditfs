CC=9c
LD=9l
CFLAGS=
LDFLAGS=-lcurl -ljansson

all:V:	reddit_client

reddit_client:	redditfs.o reddit_client.o
	$LD $LDFLAGS -o reddit_client $prereq

reddit_client.o:	reddit_client.h

%.o:	%.c
	$CC	$CFLAGS -c $stem.c

clean:V:
	rm -f *.o reddit_client
	