#ifndef WATT_MATH_TYPES_H
#define WATT_MATH_TYPES_H

#include <stdint.h>

struct vec2 {
	float x;
	float y;
};

struct vec3 {
	float x;
	float y;
	float z;
};

struct vec4 {
	float x;
	float y;
	float z;
	float w;
};

struct quat {
	float x;
	float y;
	float z;
	float w;
};

/*
 * column ordered matrices
 *
 * [x.x, y.x, z.x, w.x]
 * [x.y, y.y, z.y, w.y]
 * [x.z, y.z, z.z, w.z]
 * [x.w, y.w, z.w, w.w]
 *
 */

struct mat4 {
	struct vec4 x;
	struct vec4 y;
	struct vec4 z;
	struct vec4 w;
};

typedef struct mat4 mat4;

#endif
