#ifndef __REDDIT_CLIENT_H__
#define __REDDIT_CLIENT_H__ 1

struct rc_error
{
	char*	message;
};


struct rc_post
{
	char*	title;
	char*	url;
	long	score;
};

struct rc_post** rc_get(const char*, struct rc_error*);
#endif