#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "reddit_client.h"

typedef struct Response Response;

struct Response
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

static char*	strposts(const char*);

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
	Response *response;
	Qid q;
	
	q.type = QTDIR;
	q.vers = 0;
	q.path = 0;
	r->ofcall.qid = q;
	r->fid->qid = q;
	
	response = emalloc9p(sizeof(Response));
	r->fid->aux = response;
	respond(r, nil);
}

static char*
xwalk1(Fid *fid, char *name, Qid *qid)
{
	Response *response;
	Qid q;
	/* FIXME: check if walking subpath */
	response = fid->aux;
	response->src = estrdup9p(name);
	q.type = 0;
	q.vers = 0;
	q.path = 1;
	*qid = q;
	fid->qid = q;
	return nil;
}

static char*
xclone(Fid *fid, Fid *newfid)
{
	Response *r, *nr;
	if(fid->aux == nil)
		return nil;
	r = fid->aux;
	nr = emalloc9p(sizeof(Response));
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
	Response *response;
	if(r->ifcall.mode != OREAD){
		respond(r, "permission denied");
		return;
	}
	r->ofcall.qid = r->fid->qid;
	response = r->fid->aux;
	response->data = strposts(response->src);
	response->len  = strlen(response->data);
	respond(r, nil);
}

static void
xread(Req *r)
{
	Response *response;
	response = r->fid->aux;
	if(r->ifcall.offset >= response->len){
		r->ofcall.count = 0;
		respond(r, nil);
		return;
	}
	/*FIXME*/
	r->ofcall.data = response->data;
	r->ofcall.count = response->len;
	respond(r, nil);
}

static void
xdestroyfid(Fid* fid)
{
	Response *response;
	if(fid->aux == nil)
		return;
	response = fid->aux;
	free(response->src);
	free(response->data);
	free(response);
}

static char*
strposts(const char* name)
{
	char buffer[4096];
	int sz;
	Post** posts;
	Error error;

	sz = 0;
	posts = getposts(name, &error);
	if(posts == nil)
		return estrdup9p(error.message);

	for(int i = 0; posts[i] != nil; i++){
		sz += sprint(buffer+sz, "%ld - %s\n  %s\n",
			posts[i]->score,
			posts[i]->title,
			posts[i]->url);
		if(sz >= 4095)
			break;
	}
	return estrdup9p(buffer);
}