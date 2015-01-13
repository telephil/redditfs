#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "reddit_client.h"

static void fsattach(Req*);
static void fsopen(Req*);
static void fsread(Req*);

Srv rsrv;

void
srvinit(void)
{
	rsrv.attach = fsattach;
	rsrv.open	= fsopen;
	rsrv.read	= fsread;
}

static void
fsattach(Req *r)
{
}

static void
fsopen(Req *r)
{
}

static void
fsread(Req *r)
{
}
