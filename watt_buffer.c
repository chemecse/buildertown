#include "watt_buffer.h"

#include <stdlib.h> /* calloc, free */
#include <fcntl.h>	/* open */
#include <unistd.h> /* read, close */
#include <sys/stat.h> /* fstat */
#include <string.h> /* memset */
#include <assert.h> /* assert */

struct buffer buffer_create(uint32_t size)
{
	struct buffer result = {0};
	result.data = calloc((size_t)size, 1);
	if (result.data) {
		result.size = size;
	}
	return result;
}

struct buffer buffer_create_from_file(const char *filename)
{
	struct buffer result = {0};

	int file = open(filename, O_RDONLY);
	if (file < 0) {
		return result;
	}

	struct stat desc;
	if (!(fstat(file, &desc) == 0 && desc.st_size > 0)) {
		close(file);
		return result;
	}

	result = buffer_create(desc.st_size);
	if (result.size == 0) {
		close(file);
		return result;
	}

	ssize_t size = read(file, result.data, (size_t)result.size);
	if (size != (ssize_t)result.size) {
		buffer_destroy(&result);
	}

	close(file);
	return result;
}

void buffer_clear(struct buffer *buf)
{
	assert(buf && buf->data);
	memset(buf->data, 0, buf->size);
}

void buffer_destroy(struct buffer *buf)
{
	assert(buf && buf->data);
	free(buf->data);
	buf->data = 0;
	buf->size = 0;
}
