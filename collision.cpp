
#define COLLISION_EPSILON 0.00001f

#define FAST_ABS(x) (fabs(x))

struct Ray
{
	Vec3 origin;
	Vec3 direction;
};

Ray ray_to_dir(const Vec3& origin, const Vec3& direction)
{
	Ray ray;
	ray.origin = origin;
	ray.direction = direction;
	return ray;
}

Ray ray_to_point(const Vec3& a, const Vec3& b)
{
	return ray_to_dir(a, b - a);
}

Vec3 ray_at(const Ray& ray, float t)
{
	return ray.origin + ray.direction * t;
}

struct Line_T2
{
	float t1;
	float t2;
	int parallel;
};

Line_T2 closest_points_on_lines(Vec3 p1, Vec3 d1, Vec3 p2, Vec3 d2)
{
	// Real-Time Collision Detection 5.1.18
	Vec3 r = p1 - p2;

	float d11 = dot(d1, d1);
	float d12 = dot(d1, d2);
	float d22 = dot(d2, d2);

	float r1 = dot(d1, r);
	float r2 = dot(d2, r);

	float det = d11*d22 - d12*d12;

	Line_T2 ret;

	// Note: det >= 0
	if (det > COLLISION_EPSILON) {
		float invdet = 1.0f / det;
		ret.t1 = (d12*r2 - d22*r1) / det;
		ret.t2 = (d11*r2 - d12*r1) / det;
		ret.parallel = 0;
	} else {
		ret.t1 = 0.0f;
		ret.t2 = dot(p1 - p2, d2);
		ret.parallel = 1;
	}

	return ret;
}

Line_T2 closest_points_on_lines(Ray r1, Ray r2)
{
	return closest_points_on_lines(r1.origin, r1.direction, r2.origin, r2.direction);
}

