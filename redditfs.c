#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

extern void srvinit(void);
extern Srv rsrv;

void
usage()
{
	fprint(2, "usage: redditfs\n");
	threadexitsall("usage");
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
	
	srvinit();
	threadpostmountsrv(&rsrv, "reddit", nil, 0);
	threadexits(nil);
}
