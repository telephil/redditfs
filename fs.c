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
	size_t	len;
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
	q.type = 0;
	q.vers = 0;
	q.path = 1;
	*qid = q;
	fid->qid = q;
	return nil;
}

static char*
xclone(Fid *oldfid, Fid *newfid)
{
	SubFid *r, *nr;
	if(oldfid->aux == nil)
		return nil;
	r = oldfid->aux;
	nr = emalloc9p(sizeof(SubFid));
	if(r->src != nil)
		nr->src = estrdup9p(r->src);
	if(r->data != nil)
		nr->data = estrdup9p(r->data);
	nr->len = r->len;
	newfid->aux = nr;
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
	if(r->ifcall.offset >= sf->len){
		r->ofcall.count = 0;
		respond(r, nil);
		return;
	}
	/* FIXME: some cases might fail!!! */
	if(r->ifcall.count < sf->len){
		char *s = emalloc9p(r->ifcall.count);
		strncpy(s, sf->data+r->ifcall.offset, r->ifcall.count);
		r->ofcall.data = s;
		r->ofcall.count = r->ifcall.count;
	}else{
		r->ofcall.data = sf->data;
		r->ofcall.count = sf->len;
	}
	respond(r, nil);
}

static void
xdestroyfid(Fid* fid)
{
	SubFid *sf;
	if(fid->aux == nil)
		return;
	sf = fid->aux;
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
	char 	*s;

	posts = getposts(sf->src, &error);
	if(posts == nil)
		return estrdup9p(error.message);

	str = s_newalloc(1024);
	posts_foreach(post, posts) {
		s = smprint("%ld - %s\n  %s\n",
			post->score,
			post->title,
			post->url);
		s_append(str, s);
		free(s);
	}
	sf->data = estrdup9p(s_to_c(str));
	sf->len	 = s_len(str);
	s_free(str);
	
	return nil;
}
