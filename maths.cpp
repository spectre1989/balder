#include "maths.h"

Quat quat_euler(Vec_3f euler)
{
	Quat pitch = quat_angle_axis({ 1.0f, 0.0f, 0.0f }, euler.x);
	Quat yaw = quat_angle_axis({ 0.0f, 1.0f, 0.0f }, euler.y);
	Quat roll = quat_angle_axis({ 0.0f, 0.0f, 1.0f }, euler.z);

	// do roll, then pitch, then yaw
	return quat_mul(yaw, quat_mul(pitch, roll));
}

Quat quat_angle_axis(Vec_3f axis, float32 angle)
{
	float32 half_theta = angle * 0.5f;
	float32 sin_half_theta = sinf(half_theta);
	return { axis.x * sin_half_theta, axis.y * sin_half_theta, axis.z * sin_half_theta, cosf(half_theta) };
}

void matrix_4x4_projection(
	Matrix_4x4* matrix,
	float32 fov_y,
	float32 aspect_ratio,
	float32 near_plane,
	float32 far_plane)
{
	// create projection matrix
	// Note: NDC coordinates are top-left corner (-1, -1), z 0-1
	// 1/(tan(fovx/2)*aspect)	0				0	0
	// 0						-1/tan(fovy/2)	0	0
	// 0						0				c2	c1
	// 0						0				1	0
	// this is stored column major
	// NDC Z = c1/w + c2
	// c1 = (near*far)/(near-far)
	// c2 = far/(far-near)
	* matrix = {};
	matrix->m11 = 1.0f / (tanf(fov_y * 0.5f) * aspect_ratio);
	matrix->m22 = -1.0f / tanf(fov_y * 0.5f);
	matrix->m33 = (far_plane / (far_plane - near_plane));
	matrix->m43 = 1.0f;
	matrix->m34 = (near_plane * far_plane) / (near_plane - far_plane);
}

void matrix_4x4_translation(Matrix_4x4* matrix, Vec_3f translation)
{
	matrix->m11 = 1.0f;
	matrix->m21 = 0.0f;
	matrix->m31 = 0.0f;
	matrix->m41 = 0.0f;
	matrix->m12 = 0.0f;
	matrix->m22 = 1.0f;
	matrix->m32 = 0.0f;
	matrix->m42 = 0.0f;
	matrix->m13 = 0.0f;
	matrix->m23 = 0.0f;
	matrix->m33 = 1.0f;
	matrix->m43 = 0.0f;
	matrix->m14 = translation.x;
	matrix->m24 = translation.y;
	matrix->m34 = translation.z;
	matrix->m44 = 1.0f;
}

void matrix_4x4_mul(Matrix_4x4* result, Matrix_4x4* a, Matrix_4x4* b)
{
	assert(result != a && result != b);
	result->m11 = (a->m11 * b->m11) + (a->m12 * b->m21) + (a->m13 * b->m31) + (a->m14 * b->m41);
	result->m21 = (a->m21 * b->m11) + (a->m22 * b->m21) + (a->m23 * b->m31) + (a->m24 * b->m41);
	result->m31 = (a->m31 * b->m11) + (a->m32 * b->m21) + (a->m33 * b->m31) + (a->m34 * b->m41);
	result->m41 = (a->m41 * b->m11) + (a->m42 * b->m21) + (a->m43 * b->m31) + (a->m44 * b->m41);
	result->m12 = (a->m11 * b->m12) + (a->m12 * b->m22) + (a->m13 * b->m32) + (a->m14 * b->m42);
	result->m22 = (a->m21 * b->m12) + (a->m22 * b->m22) + (a->m23 * b->m32) + (a->m24 * b->m42);
	result->m32 = (a->m31 * b->m12) + (a->m32 * b->m22) + (a->m33 * b->m32) + (a->m34 * b->m42);
	result->m42 = (a->m41 * b->m12) + (a->m42 * b->m22) + (a->m43 * b->m32) + (a->m44 * b->m42);
	result->m13 = (a->m11 * b->m13) + (a->m12 * b->m23) + (a->m13 * b->m33) + (a->m14 * b->m43);
	result->m23 = (a->m21 * b->m13) + (a->m22 * b->m23) + (a->m23 * b->m33) + (a->m24 * b->m43);
	result->m33 = (a->m31 * b->m13) + (a->m32 * b->m23) + (a->m33 * b->m33) + (a->m34 * b->m43);
	result->m43 = (a->m41 * b->m13) + (a->m42 * b->m23) + (a->m43 * b->m33) + (a->m44 * b->m43);
	result->m14 = (a->m11 * b->m14) + (a->m12 * b->m24) + (a->m13 * b->m34) + (a->m14 * b->m44);
	result->m24 = (a->m21 * b->m14) + (a->m22 * b->m24) + (a->m23 * b->m34) + (a->m24 * b->m44);
	result->m34 = (a->m31 * b->m14) + (a->m32 * b->m24) + (a->m33 * b->m34) + (a->m34 * b->m44);
	result->m44 = (a->m41 * b->m14) + (a->m42 * b->m24) + (a->m43 * b->m34) + (a->m44 * b->m44);
}

Vec_3f matrix_4x4_mul(Matrix_4x4* matrix, Vec_3f v)
{
	return { (v.x * matrix->m11) + (v.y * matrix->m12) + (v.z * matrix->m13) + matrix->m14,
		(v.x * matrix->m21) + (v.y * matrix->m22) + (v.z * matrix->m23) + matrix->m24,
		(v.x * matrix->m31) + (v.y * matrix->m32) + (v.z * matrix->m33) + matrix->m34};
}

Vec_3f matrix_4x4_mul_direction(Matrix_4x4* matrix, Vec_3f v)
{
	return { (v.x * matrix->m11) + (v.y * matrix->m12) + (v.z * matrix->m13),
		(v.x * matrix->m21) + (v.y * matrix->m22) + (v.z * matrix->m23),
		(v.x * matrix->m31) + (v.y * matrix->m32) + (v.z * matrix->m33) };
}

Vec_4f matrix_4x4_mul_vec4(Matrix_4x4* matrix, Vec_3f v)
{
	return { (v.x * matrix->m11) + (v.y * matrix->m12) + (v.z * matrix->m13) + matrix->m14,
		(v.x * matrix->m21) + (v.y * matrix->m22) + (v.z * matrix->m23) + matrix->m24,
		(v.x * matrix->m31) + (v.y * matrix->m32) + (v.z * matrix->m33) + matrix->m34,
		(v.x * matrix->m41) + (v.y * matrix->m42) + (v.z * matrix->m43) + matrix->m44 };
}

void matrix_4x4_camera(Matrix_4x4* matrix, Vec_3f position, Vec_3f forward, Vec_3f up, Vec_3f right)
{
	// need to use camera position as the effective origin,
	// so negate position to give that translation
	Vec_3f translation = vec_3f_mul(position, -1.0f);

	matrix->m11 = right.x;
	matrix->m21 = up.x;
	matrix->m31 = forward.x;
	matrix->m41 = 0.0f;
	matrix->m12 = right.y;
	matrix->m22 = up.y;
	matrix->m32 = forward.y;
	matrix->m42 = 0.0f;
	matrix->m13 = right.z;
	matrix->m23 = up.z;
	matrix->m33 = forward.z;
	matrix->m43 = 0.0f;
	matrix->m14 = (right.x * translation.x) + (right.y * translation.y) + (right.z * translation.z);
	matrix->m24 = (up.x * translation.x) + (up.y * translation.y) + (up.z * translation.z);
	matrix->m34 = (forward.x * translation.x) + (forward.y * translation.y) + (forward.z * translation.z);
	matrix->m44 = 1.0f;
}

void matrix_4x4_lookat(Matrix_4x4* matrix, Vec_3f position, Vec_3f target, Vec_3f up)
{
	Vec_3f view_forward = vec_3f_normalised(vec_3f_sub(target, position));
	Vec_3f project_up_onto_forward = vec_3f_mul(view_forward, vec_3f_dot(up, view_forward));
	Vec_3f view_up = vec_3f_normalised(vec_3f_sub(up, project_up_onto_forward));
	Vec_3f view_right = vec_3f_cross(view_forward, view_up);
	Vec_3f translation = vec_3f_mul(position, -1.0f);

	matrix_4x4_camera(matrix, position, view_forward, view_up, view_right);
}

void matrix_4x4_rotation(Matrix_4x4* matrix, Quat rotation)
{
	// x axis
	// we can be faster than quat_mul because we know x is 1, and y/z is 0
	matrix->m11 = (rotation.scalar * rotation.scalar) + (rotation.zy * rotation.zy) + (-1.0f * rotation.xz * rotation.xz) + (-1.0f * rotation.yx * rotation.yx);
	matrix->m21 = (2.0f * rotation.scalar * rotation.yx) + (2.0f * rotation.zy * rotation.xz);
	matrix->m31 = (-2.0f * rotation.scalar * rotation.xz) + (2.0f * rotation.zy * rotation.yx);
	matrix->m41 = 0.0f;

	// y axis
	matrix->m12 = (-2.0f * rotation.scalar * rotation.yx) + (2.0f * rotation.zy * rotation.xz);
	matrix->m22 = (rotation.scalar * rotation.scalar) + (-1.0f * rotation.zy * rotation.zy) + (rotation.xz * rotation.xz) + (-1.0f * rotation.yx * rotation.yx);
	matrix->m32 = (2.0f * rotation.scalar * rotation.zy) + (2.0f * rotation.xz * rotation.yx);
	matrix->m42 = 0.0f;

	// z axis
	matrix->m13 = (2.0f * rotation.scalar * rotation.xz) + (2.0f * rotation.zy * rotation.yx);
	matrix->m23 = (-2.0f * rotation.scalar * rotation.zy) + (2.0f * rotation.xz * rotation.yx);
	matrix->m33 = (rotation.scalar * rotation.scalar) + (-1.0f * rotation.zy * rotation.zy) + (-1.0f * rotation.xz * rotation.xz) + (rotation.yx * rotation.yx);
	matrix->m43 = 0.0f;

	// translation
	matrix->m14 = 0.0f;
	matrix->m24 = 0.0f;
	matrix->m34 = 0.0f;
	matrix->m44 = 1.0f;
}

void matrix_4x4_transform(Matrix_4x4* matrix, Vec_3f position, Quat rotation)
{
	// rotation THEN translation
	matrix_4x4_rotation(matrix, rotation);

	// rotation matrix is orthogonal 3x3, so it's easy to add the translation
	matrix->m14 = position.x;
	matrix->m24 = position.y;
	matrix->m34 = position.z;
}