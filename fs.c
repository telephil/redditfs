#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <libString.h>
#include "reddit_client.h"

typedef struct SubFid SubFid;

struct SubFid
{
	char*	src;
	char*	data;
};

static void 	xattach(Req*);
static void 	xopen(Req*);
static char* 	xwalk1(Fid*, char*, Qid*);
static char*	xclone(Fid*, Fid*);
static void 	xread(Req*);
static void		xdestroyfid(Fid*);

static char*	readsub(SubFid*);

Srv xsrv;

void
xinit(void)
{
	xsrv.attach		= xattach;
	xsrv.open		= xopen;
	xsrv.walk1		= xwalk1;
	xsrv.clone		= xclone;
	xsrv.read		= xread;
	xsrv.destroyfid	= xdestroyfid;
}

static void
xattach(Req *r)
{
	SubFid *sf;
	Qid q;
	
	q.type = QTDIR;
	q.vers = 0;
	q.path = 0;
	r->ofcall.qid = q;
	r->fid->qid = q;
	
	sf = emalloc9p(sizeof(SubFid));
	r->fid->aux = sf;
	respond(r, nil);
}

static char*
xwalk1(Fid *fid, char *name, Qid *qid)
{
	SubFid *sf;
	Qid q;
	/* FIXME: check if walking subpath */
	sf = fid->aux;
	sf->src = estrdup9p(name);
	q.type = QTFILE;
	q.vers = 0;
	q.path = 1;
	*qid = q;
	fid->qid = q;
	return nil;
}

static char*
xclone(Fid *oldfid, Fid *newfid)
{
	SubFid *sf, *nsf;
	sf = oldfid->aux;
	if(sf == nil)
		return nil;
	nsf = emalloc9p(sizeof(SubFid));
	if(sf->src != nil)
		nsf->src = estrdup9p(sf->src);
	if(sf->data != nil)
		nsf->data = estrdup9p(sf->data);
	newfid->aux = nsf;
	return nil;
}

static void
xopen(Req *r)
{
	char *err;
	if(r->ifcall.mode != OREAD){
		respond(r, "permission denied");
		return;
	}
	r->ofcall.qid = r->fid->qid;
	err = readsub(r->fid->aux);
	respond(r, err);
}

static void
xread(Req *r)
{
	SubFid *sf;
	sf = r->fid->aux;
	readstr(r, sf->data);
	respond(r, nil);
}

static void
xdestroyfid(Fid* fid)
{
	SubFid *sf;
	sf = fid->aux;
	if(sf == nil)
		return;
	free(sf->src);
	free(sf->data);
	free(sf);
}

static char*
readsub(SubFid *sf)
{
	String 	*str;
	Post	**posts;
	Post	*post;
	Error	error;
	char 	buf[1024];

	posts = getposts(sf->src, &error);
	if(posts == nil)
		return estrdup9p(error.message);

	str = s_newalloc(1024);
	posts_foreach(post, posts) {
		snprint(buf, 1024, "%ld - %s\n  %s\n",
			post->score,
			post->title,
			post->url);
		s_append(str, buf);
	}
	sf->data = estrdup9p(s_to_c(str));
	s_free(str);
	
	return nil;
}
