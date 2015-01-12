#include <u.h>
#include <libc.h>

/* Undefine p9 aliases as it conflicts with CURL */
#undef listen
#undef accept
#include <curl/curl.h>
#include <jansson.h>

#include "reddit_client.h"

/* Constants */
#define URL_FORMAT	"http://reddit.com/r/%s/.json"
#define HTTP_OK		200

/* Memory buffer to hold HTTP response */
typedef struct Buffer Buffer;

struct Buffer
{
	char*	data;
	size_t	size;
};

/* Local funcs */
static void seterror(Error*, char*, ...);
static Post* getpost(json_t*, Error*);
static char* httpget(const char*, Error*);

/* Retrieve list of posts from given subreddit */
Post**
getposts(const char* subreddit, Error* error)
{
	json_error_t err;
	json_t *root, *data, *children, *child, *node;
	Post **result = nil;
	size_t size;
	size_t index;
	char *url;
	
	if(subreddit == nil || strlen(subreddit) == 0) {
		seterror(error, "subreddit name cannot be null or empty");
		return nil;
	}
	
	url = smprint(URL_FORMAT, subreddit);
	
	char* buffer = httpget(url, error);
	if(buffer == nil) {
		return nil;
	}
	
	root = json_loads(buffer, 0, &err);
	free(buffer);
	
	if(!root) {
    	seterror(error, "error: on line %d: %s", err.line, err.text);
    	return nil;
	}
	
	data = json_object_get(root, "data");
	if(!json_is_object(data)) {
		seterror(error, "unable to get data node");
		goto error;
	}
	children = json_object_get(data, "children");
	if(!json_is_array(children)) {
		seterror(error, "unable to get children list");
		goto error;
	}
	size = json_array_size(children);
	result = calloc(size + 1, sizeof(Post*));
	if(result == nil) {
		seterror(error, "unable to allocate memory for result list");
		goto error;
	}
	json_array_foreach(children, index, child) {
		node = json_object_get(child, "data");
		if(!json_is_object(node)) {
			seterror(error, "unable to get child data");
			goto error;
		}
		Post* post = getpost(node, error);
		if(post == nil) {
			goto error;
		}
		
		result[index] = post;
	}
	result[size] = nil;

	json_decref(root);
	return result;

error:
	if(result)
		free(result);
	json_decref(root);
	return nil;
}

static
Post*
getpost(json_t *node, Error* error)
{
	json_t *title, *url, *score;
	Post* post = nil;
	
	title = json_object_get(node, "title");
	if(!json_is_string(title)) {
		seterror(error, "title has invalid element type");
		return nil;
	}
	url = json_object_get(node, "url");
	if(!json_is_string(url)) {
		seterror(error, "url has invalid element type");
		return nil;
	}
	score = json_object_get(node, "score");
	if(!json_is_integer(score)) {
		seterror(error, "score has invalid element type");
		return nil;
	}

	post = malloc(sizeof(Post));
	if(post == nil) {
		seterror(error, "out of memory");
		return nil;
	}
	
	post->title = strdup((char*)json_string_value(title));
	post->url   = strdup((char*)json_string_value(url));
	post->score = (long)json_integer_value(score);
	
	return post;
}

static
size_t
writecallback(void* contents, size_t size, size_t nmemb, void* data)
{
	size_t sz = nmemb * size;
	Buffer* buffer = (Buffer*)data;
	
	buffer->data = realloc(buffer->data, buffer->size + sz + 1);
	if(buffer->data == nil) {
		fprint(2, "unable to allocate memory for http response buffer\n");
		return 0;
	}
	
	memcpy(buffer->data + buffer->size, contents, sz);
	buffer->size += sz;
	buffer->data[buffer->size] = 0;
	
	return sz;
}

static 
char*
httpget(const char* url, Error* error)
{
	CURL* curl = nil;
	CURLcode rc;
	long code;
	char* data = nil;
	Buffer buffer;
	
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if(!curl)
		return nil;
	
	buffer.size = 0;
	buffer.data = malloc(1);
	if(!buffer.data) {
		seterror(error, "out of memory");
		goto cleanup;
	}
	
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writecallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&buffer);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	
	rc = curl_easy_perform(curl);
	if(rc != 0) {
		seterror(error, "unable to request '%s' (%s)", url, curl_easy_strerror(rc));
		goto cleanup;
	}
	
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if(code != HTTP_OK) {
        seterror(error, "server responded with code %ld", code);
		goto cleanup;
    }
	
	data = strdup(buffer.data);
	free(buffer.data);

cleanup:
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	
	return data;
}


static
void
seterror(Error* error, char* fmt, ...)
{
	if(error == nil)
		return;

	va_list ap;
	va_start(ap, fmt);
	error->message = vsmprint(fmt, ap);
	if(error->message == nil) {
		fprint(2, "fatal: unable to allocate memory for error message\n");
		exits("oom");
	}
	va_end(ap);
}

