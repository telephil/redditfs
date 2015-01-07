#include <stdio.h>
#include "reddit_client.h"

int
main(int argc, const char* argv[])
{
	const char* sub = argc == 2 ? argv[1] : "plan9";
	struct rc_error error;
	struct rc_post** posts = rc_get(sub, &error);
	if(posts == NULL) {
		fprintf(stderr, "error: %s\n", error.message);
		return 1;
	}
	for(int i = 0; posts[i] != NULL; i++) {
		printf("[%ld] %s\n => %s\n", 
			posts[i]->score,
			posts[i]->title,
			posts[i]->url);
	}
	return 0;
}