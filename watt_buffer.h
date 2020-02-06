#ifndef WATT_BUFFER_H
#define WATT_BUFFER_H

#include <stdint.h>

struct buffer {
	void *data;
	uint32_t size;
};

struct buffer buffer_create(uint32_t size);
struct buffer buffer_create_from_file(const char *filename);
void buffer_destroy(struct buffer *buf);

#endif
