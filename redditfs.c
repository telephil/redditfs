#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

extern void xinit(void);
extern Srv xsrv;

void
usage()
{
	fprint(2, "usage: redditfs\n");
	threadexitsall("usage");
}

int
threadmaybackground(void)
{
	return 1;
}

void
threadmain(int argc, char *argv[])
{
	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	default:
		usage();
	}ARGEND
	
	xinit();
	threadpostmountsrv(&xsrv, "reddit", nil, 0);
	threadexits(nil);
}
