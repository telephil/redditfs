#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#include "reddit_client.h"

#define URL_FORMAT	"http://reddit.com/r/%s/.json"
#define URL_LENGTH	256
#define HTTP_OK		200

/* Memory buffer to hold HTTP response */
struct buffer
{
	char*	data;
	size_t	size;
};

static void set_error(struct rc_error*, const char*, ...);
static struct rc_post* get_post(json_t*, struct rc_error*);

static size_t
write_callback(void* contents, size_t size, size_t nmemb, void* data)
{
	size_t sz = nmemb * size;
	struct buffer* buffer = (struct buffer*)data;
	
	buffer->data = realloc(buffer->data, buffer->size + sz + 1);
	if(buffer->data == NULL) {
		fprintf(stderr, "Out of Memory\n");
		return 0;
	}
	
	memcpy(buffer->data + buffer->size, contents, sz);
	buffer->size += sz;
	buffer->data[buffer->size] = 0;
	
	return sz;
}

static char*
http_get(const char* url, struct rc_error* error)
{
	CURL* curl = NULL;
	CURLcode rc;
	long code;
	char* data = NULL;
	struct buffer buffer;
	
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if(!curl)
		return NULL;
	
	buffer.size = 0;
	buffer.data = malloc(1);
	if(!buffer.data) {
		set_error(error, "out of memory");
		goto cleanup;
	}
	
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&buffer);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	
	rc = curl_easy_perform(curl);
	if(rc != 0) {
		set_error(error, "unable to request '%s' (%s)", url, curl_easy_strerror(rc));
		goto cleanup;
	}
	
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if(code != HTTP_OK) {
        set_error(error, "server responded with code %ld\n", code);
		goto cleanup;
    }
	
	data = strdup(buffer.data);
	free(buffer.data);

cleanup:
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	
	return data;
}

struct rc_post**
rc_get(const char* subreddit, struct rc_error* error)
{
	json_error_t err;
	json_t *root, *data, *children, *child, *node;
	struct rc_post **result = NULL;
	size_t size;
	size_t index;
	char url[URL_LENGTH];
	
	if(subreddit == NULL || strlen(subreddit) == 0) {
		set_error(error, "subreddit name cannot be null or empty");
		return NULL;
	}
	
	snprintf(url, URL_LENGTH - 1, URL_FORMAT, subreddit);
	
	char* buffer = http_get(url, error);
	if(buffer == NULL) {
		return NULL;
	}
	
	root = json_loads(buffer, 0, &err);
	free(buffer);
	
	if(!root) {
    	set_error(error, "error: on line %d: %s", err.line, err.text);
    	return NULL;
	}
	
	data = json_object_get(root, "data");
	if(!json_is_object(data)) {
		set_error(error, "unable to get data node");
		goto error;
	}
	children = json_object_get(data, "children");
	if(!json_is_array(children)) {
		set_error(error, "unable to get children list");
		goto error;
	}
	size = json_array_size(children);
	result = calloc(size + 1, sizeof(struct rc_post*));
	if(result == NULL) {
		set_error(error, "unable to allocate memory for result list");
		goto error;
	}
	json_array_foreach(children, index, child) {
		node = json_object_get(child, "data");
		if(!json_is_object(node)) {
			set_error(error, "unable to get child data");
			goto error;
		}
		struct rc_post* post = get_post(node, error);
		if(post == NULL) {
			goto error;
		}
		
		result[index] = post;
	}
	result[size] = NULL;

	json_decref(root);
	return result;

error:
	if(result)
		free(result);
	json_decref(root);
	return NULL;
}

static struct rc_post*
get_post(json_t *node, struct rc_error* error)
{
	json_t *title, *url, *score;
	struct rc_post* post = NULL;
	
	title = json_object_get(node, "title");
	if(!json_is_string(title)) {
		set_error(error, "title has invalid element type");
		return NULL;
	}
	url = json_object_get(node, "url");
	if(!json_is_string(url)) {
		set_error(error, "url has invalid element type");
		return NULL;
	}
	score = json_object_get(node, "score");
	if(!json_is_integer(score)) {
		set_error(error, "score has invalid element type");
		return NULL;
	}

	post = malloc(sizeof(struct rc_post));
	if(post == NULL) {
		set_error(error, "out of memory");
		return NULL;
	}
	
	post->title = strdup(json_string_value(title));
	post->url   = strdup(json_string_value(url));
	post->score = (long)json_integer_value(score);
	
	return post;
}

static void
set_error(struct rc_error* error, const char* fmt, ...)
{
	if(error == NULL)
		return;

	int sz;
	va_list ap;
	va_start(ap, fmt);
	sz = vasprintf(&error->message, fmt, ap);
	if(sz < 0) {
		fprintf(stderr, "fatal: unable to allocate memory for error message\n");
		exit(1);
	}
	va_end(ap);
}

