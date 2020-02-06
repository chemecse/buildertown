#include "watt_math.h"

#include <math.h>   // sqrtf, sinf, cosf, tanf
#include <string.h> // memset
#include <assert.h> // assert

struct vec2 v2(float x, float y)
{
	struct vec2 v;
	v.x = x;
	v.y = y;
	return v;
}

struct vec3 v3(float x, float y, float z)
{
	struct vec3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

struct vec3 vec3_add(struct vec3 a, struct vec3 b)
{
	struct vec3 v;
	v.x = a.x + b.x;
	v.y = a.y + b.y;
	v.z = a.z + b.z;
	return v;
}

struct vec3 vec3_scale(struct vec3 a, float f)
{
	struct vec3 v;
	v.x = a.x * f;
	v.y = a.y * f;
	v.z = a.z * f;
	return v;
}

struct vec3 vec3_cross(struct vec3 a, struct vec3 b)
{
	struct vec3 v;
	v.x = a.y * b.z - a.z * b.y;
	v.y = a.z * b.x - a.x * b.z;
	v.z = a.x * b.y - a.y * b.x;
	return v;
}

struct vec3 vec3_normalize(struct vec3 a) { return (vec3_scale(a, 1.0f / vec3_length(a))); }

float vec3_dot(struct vec3 a, struct vec3 b) { return (a.x * b.x + a.y * b.y + a.z * b.z); }

float vec3_length_squared(struct vec3 a) { return vec3_dot(a, a); }

float vec3_length(struct vec3 a) { return sqrtf(vec3_length_squared(a)); }

struct vec4 v4(float x, float y, float z, float w)
{
	struct vec4 v;
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;
	return v;
}

struct quat quat_axis_angle(struct vec3 axis, float rad)
{
	struct quat result;
	float half_rad, s, c;

	half_rad = rad / 2.0f;
	s = sinf(half_rad);
	c = cosf(half_rad);

	result.x = axis.x * s;
	result.y = axis.y * s;
	result.z = axis.z * s;
	result.w = c;
	return result;
}

struct quat quat_multiply(struct quat a, struct quat b)
{
	struct quat result;
	result.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
	result.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
	result.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
	result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
	return result;
}

struct vec3 quat_rotate(struct vec3 v, struct quat q)
{
	struct vec3 result, qv, tmp;

	qv = v3(q.x, q.y, q.z);
	result = vec3_cross(qv, v);
	tmp = vec3_scale(v, q.w);
	result = vec3_add(result, tmp);
	result = vec3_cross(qv, result);
	result = vec3_scale(result, 2.0f);
	result = vec3_add(result, v);
	return result;
}

struct mat4 mat4_identity(void)
{
	struct mat4 result;
	memset(&result, 0, sizeof(struct mat4));
	result.x.x = 1.0f;
	result.y.y = 1.0f;
	result.z.z = 1.0f;
	result.w.w = 1.0f;
	return result;
}

struct mat4 mat4_add(struct mat4 m0, struct mat4 m1)
{
	struct mat4 result;
	float *a = &m0.x.x;
	float *b = &m1.x.x;
	float *c = &result.x.x;
	int32_t i, j, i4, index;

	for (i = 0; i < 4; ++i) {
		i4 = i * 4;
		for (j = 0; j < 4; ++j) {
			index = i4 + j;
			c[index] = a[index] + b[index];
		}
	}
	return result;
}

struct mat4 mat4_multiply(struct mat4 m0, struct mat4 m1)
{
	struct mat4 result;
	float *a = &m0.x.x;
	float *b = &m1.x.x;
	float *c = &result.x.x;
	int32_t i, j, i4;

	for (i = 0; i < 4; ++i) {
		i4 = i * 4;
		for (j = 0; j < 4; ++j) {
			c[i4 + j] = a[0 * 4 + j] * b[i4 + 0] + a[1 * 4 + j] * b[i4 + 1] + a[2 * 4 + j] * b[i4 + 2] +
						a[3 * 4 + j] * b[i4 + 3];
		}
	}
	return result;
}

struct mat4 mat4_multiply_scalar(struct mat4 m, float f)
{
	struct mat4 result;
	float *a = &m.x.x;
	float *b = &result.x.x;
	int32_t i, j, i4, index;

	for (i = 0; i < 4; ++i) {
		i4 = i * 4;
		for (j = 0; j < 4; ++j) {
			index = i4 + j;
			b[index] = a[index] * f;
		}
	}
	return result;
}

struct mat4 mat4_scale(struct mat4 m, struct vec3 v)
{
	struct mat4 result;
	result = mat4_identity();
	result.x.x = v.x;
	result.y.y = v.y;
	result.z.z = v.z;
	return mat4_multiply(m, result);
}

struct mat4 mat4_translate(struct mat4 m, struct vec3 v)
{
	struct mat4 result;
	result = mat4_identity();
	result.w.x = v.x;
	result.w.y = v.y;
	result.w.z = v.z;
	return mat4_multiply(m, result);
}

struct mat4 mat4_rotate_x(struct mat4 m, float rad)
{
	float c, s;
	struct mat4 result;

	c = cosf(rad);
	s = sinf(rad);

	result = mat4_identity();
	result.y.y = c;
	result.y.z = s;
	result.z.y = -s;
	result.z.z = c;
	return mat4_multiply(m, result);
}

struct mat4 mat4_rotate_y(struct mat4 m, float rad)
{
	float c, s;
	struct mat4 result;

	c = cosf(rad);
	s = sinf(rad);

	result = mat4_identity();
	result.x.x = c;
	result.x.z = -s;
	result.z.x = s;
	result.z.z = c;
	return mat4_multiply(m, result);
}

struct mat4 mat4_rotate_z(struct mat4 m, float rad)
{
	float c, s;
	struct mat4 result;

	c = cosf(rad);
	s = sinf(rad);

	result = mat4_identity();
	result.x.x = c;
	result.x.y = s;
	result.y.x = -s;
	result.y.y = c;
	return mat4_multiply(m, result);
}

struct mat4 mat4_look_at(struct vec3 look_from, struct vec3 look_dir, struct vec3 look_up)
{
	struct vec3 right_dir, up_dir;
	struct mat4 result;

	look_dir = vec3_normalize(look_dir);
	right_dir = vec3_normalize(vec3_cross(look_dir, vec3_normalize(look_up)));
	up_dir = vec3_cross(right_dir, look_dir);

	result = mat4_identity();
	result.x.x = right_dir.x;
	result.y.x = right_dir.y;
	result.z.x = right_dir.z;
	result.x.y = up_dir.x;
	result.y.y = up_dir.y;
	result.z.y = up_dir.z;
	result.x.z = -look_dir.x;
	result.y.z = -look_dir.y;
	result.z.z = -look_dir.z;
	result.w.x = -vec3_dot(right_dir, look_from);
	result.w.y = -vec3_dot(up_dir, look_from);
	result.w.z = vec3_dot(look_dir, look_from);
	return result;
}

struct mat4 mat4_perspective(float fov, float aspect, float z_near, float z_far)
{
	float y, x, z_range;
	struct mat4 result;

	assert(aspect != 0.0f);
	assert(z_near != z_far);

	y = 1.0f / tanf(fov / 2.0f);
	x = y / aspect;
	z_range = z_far - z_near;

	memset(&result, 0, sizeof(struct mat4));
	result.x.x = x;
	result.y.y = y;
	result.z.z = -(z_far + z_near) / z_range;
	result.z.w = -1.0f;
	result.w.z = (-2.0f * z_near * z_far) / z_range;
	return result;
}
