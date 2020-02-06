#ifndef WATT_MATH_H
#define WATT_MATH_H

#include "watt_math_types.h"

#define WATT_PI32 3.14159265359f
#define WATT_RAD_FROM_DEG(deg) (deg / 180.0f * WATT_PI32)

struct vec2 v2(float x, float y);

struct vec3 v3(float x, float y, float z);
struct vec3 vec3_add(struct vec3 a, struct vec3 b);
struct vec3 vec3_scale(struct vec3 a, float f);
struct vec3 vec3_cross(struct vec3 a, struct vec3 b);
struct vec3 vec3_normalize(struct vec3 a);
float vec3_dot(struct vec3 a, struct vec3 b);
float vec3_length_squared(struct vec3 a);
float vec3_length(struct vec3 a);

struct vec4 v4(float x, float y, float z, float w);

struct quat quat_axis_angle(struct vec3 axis, float rad);
struct quat quat_multiply(struct quat a, struct quat b);
struct vec3 quat_rotate(struct vec3 v, struct quat q);

struct mat4 mat4_identity(void);
struct mat4 mat4_add(struct mat4 m0, struct mat4 m1);
struct mat4 mat4_multiply(struct mat4 m0, struct mat4 m1);
struct mat4 mat4_multiply_scalar(struct mat4 m, float f);
struct mat4 mat4_scale(struct mat4 m, struct vec3 v);
struct mat4 mat4_translate(struct mat4 m, struct vec3 v);
struct mat4 mat4_rotate_x(struct mat4 m, float rad);
struct mat4 mat4_rotate_y(struct mat4 m, float rad);
struct mat4 mat4_rotate_z(struct mat4 m, float rad);
struct mat4 mat4_look_at(struct vec3 look_from, struct vec3 look_dir, struct vec3 look_up);
struct mat4 mat4_perspective(float fov, float aspect, float z_near, float z_far);

#endif
