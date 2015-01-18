#ifndef __REDDIT_CLIENT_H__
#define __REDDIT_CLIENT_H__ 1

typedef struct Error Error;
typedef struct Post  Post;

struct Error
{
	char*	message;
};


struct Post
{
	char*	title;
	char*	url;
	long	score;
};

/* Retrieve list of Post for given subreddit
 * Return nil on error and set error message
 */
Post** getposts(const char*, Error*);

#define posts_foreach(P,PP) for(int i = 0; (P = PP[i]) != nil; i++)

#endif
