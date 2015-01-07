#include <u.h>
#include <libc.h>
#include "reddit_client.h"

void
main(int argc, const char* argv[])
{
	const char* sub = argc == 2 ? argv[1] : "plan9";
	struct rc_error error;
	struct rc_post** posts = rc_get(sub, &error);
	if(posts == NULL) {
		fprint(2, "error: %s\n", error.message);
		exits(error.message);
	}
	for(int i = 0; posts[i] != NULL; i++) {
		print("[%ld] %s\n => %s\n", 
			posts[i]->score,
			posts[i]->title,
			posts[i]->url);
	}
	exits(0);
}
