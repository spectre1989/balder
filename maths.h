#pragma once

#include "core.h"
#include <cmath>

constexpr float32 c_pi = 3.1415926535897932384626433f;
constexpr float32 c_deg_to_rad = c_pi / 180.0f;


union Vec_2f
{
	struct {
		float32 x, y;
	};
	float v[2];
};

union Vec_3f
{
	struct {
		float32 x, y, z;
	};
	float v[3];
};

union Vec_4f
{
	struct {
		float32 x, y, z, w;
	};
	float v[4];
};

union Quat
{
	struct {
		float32 zy, xz, yx, scalar;
	};
	struct {
		float32 x, y, z, w;
	};
};

struct Transform
{
	Vec_3f position;
	Quat rotation;
};

struct Matrix_4x4
{
	// m11 m12 m13 m14
	// m21 m22 m23 m24
	// m31 m32 m33 m34
	// m41 m42 m43 m44

	// column major
	float32 m11, m21, m31, m41,
		m12, m22, m32, m42,
		m13, m23, m33, m43,
		m14, m24, m34, m44;
};

constexpr float32 float32_abs(float32 f)
{
	return f >= 0.0f ? f : -f;
}

constexpr float32 float32_min(float32 a, float32 b)
{
	return a < b ? a : b;
}

constexpr float32 float32_max(float32 a, float32 b)
{
	return a > b ? a : b;
}

constexpr float32 float32_lerp(float32 a, float32 b, float32 t)
{
	return a + ((b - a) * t);
}

constexpr int32 int32_abs(int32 a)
{
	return a >= 0 ? a : -a;
}

constexpr int32 int32_min(int32 a, int32 b)
{
	return a < b ? a : b;
}

constexpr int32 int32_max(int32 a, int32 b)
{
	return a > b ? a : b;
}

constexpr Vec_3f vec_3f_sub(Vec_3f a, Vec_3f b)
{
	return {a.x - b.x, a.y - b.y, a.z - b.z};
}

constexpr Vec_3f vec_3f_mul(Vec_3f a, Vec_3f b)
{
	return {a.x * b.x, a.y * b.y, a.z * b.z};
}

constexpr Vec_3f vec_3f_mul(Vec_3f v, float32 f)
{
	return { v.x * f, v.y * f, v.z * f };
}

constexpr float32 vec_3f_dot(Vec_3f a, Vec_3f b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

constexpr Vec_3f vec_3f_cross(Vec_3f a, Vec_3f b)
{
	return { (a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x) };
}

constexpr Vec_3f vec_3f_lerp(Vec_3f a, Vec_3f b, float32 t)
{
	return { float32_lerp(a.x, b.x, t),
		float32_lerp(a.y, b.y, t),
		float32_lerp(a.z, b.z, t) };
}

constexpr Vec_3f vec_3f_normalised(Vec_3f v)
{
	const float32 length_sq = (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
	if (length_sq > 0.0f)
	{
		return { v.x / length_sq, v.y / length_sq, v.z / length_sq };
	}
	return v;
}

Quat quat_euler(Vec_3f euler);
Quat quat_angle_axis(Vec_3f axis, float32 angle);

constexpr Quat quat_identity()
{
	return { 0.0f, 0.0f, 0.0f, 1.0f };
}

constexpr Vec_3f quat_mul(Quat q, Vec_3f v)
{
	// TODO simplify

	float32 x = (q.scalar * q.scalar * v.x) + (-2.0f * q.scalar * q.yx * v.y) + (2.0f * q.scalar * -q.xz * v.z) + (-q.zy * -q.zy * v.x) + (2.0f * -q.zy * -q.xz * v.y) + (2.0f * -q.zy * q.yx * v.z) + (-1.0f * -q.xz * -q.xz * v.x) + (-1.0f * q.yx * q.yx * v.x);
	float32 y = (2.0f * q.scalar * q.yx * v.x) + (q.scalar * q.scalar * v.y) + (-2.0f * q.scalar * -q.zy * v.z) + (2.0f * -q.zy * -q.xz * v.x) + (-1.0f * -q.zy * -q.zy * v.y) + (-q.xz * -q.xz * v.y) + (2.0f * -q.xz * q.yx * v.z) + (-1.0f * q.yx * q.yx * v.y);
	float32 z = (-2.0f * q.scalar * -q.xz * v.x) + (2.0f * q.scalar * -q.zy * v.y) + (q.scalar * q.scalar * v.z) + (2.0f * -q.zy * q.yx * v.x) + (-1.0f * -q.zy * -q.zy * v.z) + (2.0f * -q.xz * q.yx * v.y) + (-1.0f * -q.xz * -q.xz * v.z) + (q.yx * q.yx * v.z);

	return { x, y, z };
}

constexpr Quat quat_mul(Quat a, Quat b)
{
	// TODO simplify

	float32 zy = (b.zy * a.scalar) + (b.scalar * a.zy) + (b.yx * a.xz) + (-1.0f * a.yx * b.xz);
	float32 xz = (b.xz * a.scalar) + (-1.0f * b.yx * a.zy) + (b.scalar * a.xz) + (a.yx * b.zy);
	float32 yx = (b.yx * a.scalar) + (b.xz * a.zy) + (-1.0f * b.zy * a.xz) + (a.yx * b.scalar);
	float32 scalar = (b.scalar * a.scalar) + (-1.0f * b.zy * a.zy) + (-1.0f * b.xz * a.xz) + (-1.0f * a.yx * b.yx);

	return { zy, xz, yx, scalar };
}

void matrix_4x4_projection(Matrix_4x4* matrix, float32 fov_y, float32 aspect_ratio, float32 near_plane, float32 far_plane);
void matrix_4x4_translation(Matrix_4x4* matrix, Vec_3f translation);
void matrix_4x4_mul(Matrix_4x4* result, const Matrix_4x4* a, const Matrix_4x4* b);
// matrix * {x, y, z, 1}
Vec_3f matrix_4x4_mul(const Matrix_4x4* matrix, Vec_3f v);
// matrix * {x, y, z, 0}
Vec_3f matrix_4x4_mul_direction(const Matrix_4x4* matrix, Vec_3f v);
// matrix * {x, y, z, 1}
Vec_4f matrix_4x4_mul_vec4(const Matrix_4x4* matrix, Vec_3f v);
void matrix_4x4_camera(Matrix_4x4* matrix, Vec_3f position, Vec_3f forward, Vec_3f up, Vec_3f right);
void matrix_4x4_lookat(Matrix_4x4* matrix, Vec_3f position, Vec_3f target, Vec_3f up);
void matrix_4x4_rotation(Matrix_4x4* matrix, Quat rotation);
void matrix_4x4_transform(Matrix_4x4* matrix, Vec_3f position, Quat rotation); // for an object in world with position and rotation, equivalent to doing rotation, then position translation