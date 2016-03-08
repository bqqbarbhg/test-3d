#include <math.h>

struct Mat44
{
	union {
		float data[16];
		struct {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
	};
};

struct Vec2
{
	float x, y;
};

struct Vec3
{
	float x, y, z;
};

Vec3 operator+(const Vec3& a, const Vec3& b)
{
	Vec3 ret;

	ret.x = a.x + b.x;
	ret.y = a.y + b.y;
	ret.z = a.z + b.z;

	return ret;
}

Vec3& operator+=(Vec3& a, const Vec3& b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	return a;
}

Vec3 operator-(const Vec3& a, const Vec3& b)
{
	Vec3 ret;

	ret.x = a.x - b.x;
	ret.y = a.y - b.y;
	ret.z = a.z - b.z;

	return ret;
}

Vec3 operator*(const Vec3& a, const Vec3& b)
{
	Vec3 ret;

	ret.x = a.x * b.x;
	ret.y = a.y * b.y;
	ret.z = a.z * b.z;

	return ret;
}

Vec3 operator*(const Vec3& a, float b)
{
	Vec3 ret;

	ret.x = a.x * b;
	ret.y = a.y * b;
	ret.z = a.z * b;

	return ret;
}

float dot(const Vec3& a, const Vec3& b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

Vec3 cross(const Vec3& a, const Vec3& b)
{
	Vec3 ret;

	ret.x = a.y*b.z - a.z*b.y;
	ret.y = a.z*b.x - a.x*b.z;
	ret.z = a.x*b.y - a.y*b.x;

	return ret;
}

float length_squared(const Vec3& a)
{
	return a.x*a.x + a.y*a.y + a.z*a.z;
}

float length(const Vec3& a)
{
	return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
}

Vec3 normalize(const Vec3& a)
{
	return a * (1.0f / length(a));
}

const Mat44 mat44_identity = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f,
};

const Mat44 mat44_zero = {
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
};

const Vec3 vec3_zero = { 0.0f, 0.0f, 0.0f };

Mat44 mat44_translate(const Vec3& translation)
{
	Mat44 ret = mat44_identity;

	ret._14 = translation.x;
	ret._24 = translation.y;
	ret._34 = translation.z;

	return ret;
}

Mat44 mat44_scale(const Vec3& scale)
{
	Mat44 ret = mat44_identity;

	ret._11 = scale.x;
	ret._22 = scale.y;
	ret._33 = scale.z;

	return ret;
}

Mat44 mat44_rotate_x(float angle)
{
	Mat44 ret = mat44_identity;

	float cos = cosf(angle);
	float sin = sinf(angle);

	ret._22 = cos;
	ret._23 = -sin;
	ret._32 = sin;
	ret._33 = cos;

	return ret;
}

Mat44 mat44_rotate_y(float angle)
{
	Mat44 ret = mat44_identity;

	float cos = cosf(angle);
	float sin = sinf(angle);

	ret._11 = cos;
	ret._13 = sin;
	ret._31 = -sin;
	ret._33 = cos;

	return ret;
}

Mat44 mat44_rotate_z(float angle)
{
	Mat44 ret = mat44_identity;

	float cos = cosf(angle);
	float sin = sinf(angle);

	ret._11 = cos;
	ret._12 = -sin;
	ret._21 = sin;
	ret._22 = cos;

	return ret;
}

Mat44 mat44_axes(const Vec3& forward, const Vec3& right, const Vec3& up, const Vec3& pos = vec3_zero)
{
	Mat44 ret;

	ret._11 = right.x;
	ret._21 = right.y;
	ret._31 = right.z;
	ret._41 = 0.0f;

	ret._12 = up.x;
	ret._22 = up.y;
	ret._32 = up.z;
	ret._42 = 0.0f;

	ret._13 = forward.x;
	ret._23 = forward.y;
	ret._33 = forward.z;
	ret._43 = 0.0f;

	ret._14 = pos.x;
	ret._24 = pos.y;
	ret._34 = pos.z;
	ret._44 = 1.0f;

	return ret;
}

Mat44 mat44_lookat(const Vec3& eye, const Vec3& target, const Vec3& up)
{
	Vec3 forward = normalize(target - eye);
	Vec3 right = normalize(cross(forward, up));
	Vec3 new_up = cross(forward, right);

	return mat44_axes(forward, right, new_up, eye);
}

Mat44 mat44_perspective(float fov, float aspect, float near_z, float far_z)
{
	Mat44 ret = mat44_zero;

	float tan = tanf(fov / 2.0f);
	float len = far_z - near_z;

	ret._11 = 1.0f / (aspect * tan);
	ret._22 = 1.0f / (tan);
	ret._33 = -(far_z + near_z) / len;
	ret._34 = -(2 * far_z * near_z) / len;
	ret._43 = -1.0f;

	return ret;
}

Mat44 operator*(const Mat44& a, const Mat44& b)
{
	Mat44 ret;

	ret._11 = a._11*b._11 + a._21*b._12 + a._31*b._13 + a._41*b._14;
	ret._12 = a._12*b._11 + a._22*b._12 + a._32*b._13 + a._42*b._14;
	ret._13 = a._13*b._11 + a._23*b._12 + a._33*b._13 + a._43*b._14;
	ret._14 = a._14*b._11 + a._24*b._12 + a._34*b._13 + a._44*b._14;

	ret._21 = a._11*b._21 + a._21*b._22 + a._31*b._23 + a._41*b._24;
	ret._22 = a._12*b._21 + a._22*b._22 + a._32*b._23 + a._42*b._24;
	ret._23 = a._13*b._21 + a._23*b._22 + a._33*b._23 + a._43*b._24;
	ret._24 = a._14*b._21 + a._24*b._22 + a._34*b._23 + a._44*b._24;

	ret._31 = a._11*b._31 + a._21*b._32 + a._31*b._33 + a._41*b._34;
	ret._32 = a._12*b._31 + a._22*b._32 + a._32*b._33 + a._42*b._34;
	ret._33 = a._13*b._31 + a._23*b._32 + a._33*b._33 + a._43*b._34;
	ret._34 = a._14*b._31 + a._24*b._32 + a._34*b._33 + a._44*b._34;

	ret._41 = a._11*b._41 + a._21*b._42 + a._31*b._43 + a._41*b._44;
	ret._42 = a._12*b._41 + a._22*b._42 + a._32*b._43 + a._42*b._44;
	ret._43 = a._13*b._41 + a._23*b._42 + a._33*b._43 + a._43*b._44;
	ret._44 = a._14*b._41 + a._24*b._42 + a._34*b._43 + a._44*b._44;

	return ret;
}

Mat44& operator*=(Mat44& a, const Mat44& b)
{
	a = a * b;
	return a;
}

Vec3 operator*(const Vec3& vec, const Mat44& mat)
{
	Vec3 ret;

	ret.x = vec.x*mat._11 + vec.y*mat._12 + vec.z*mat._13 + mat._14;
	ret.y = vec.x*mat._21 + vec.y*mat._22 + vec.z*mat._23 + mat._24;
	ret.z = vec.x*mat._31 + vec.y*mat._32 + vec.z*mat._33 + mat._34;

	return ret;
}

