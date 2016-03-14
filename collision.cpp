
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

struct Plane
{
	Vec3 normal;
	float d;
};

Plane plane(Vec3 normal, float d)
{
	Plane plane;
	plane.normal = normal;
	plane.d = d;
	return plane;
}

Plane plane_from_point_normal(Vec3 point, Vec3 normal)
{
	float d = dot(point, normal);
	return plane(normal, d);
}

struct Line_T1
{
	float t;
	int parallel;
};

struct Line_T2
{
	float t1;
	float t2;
	int parallel;
};

Line_T2 closest_points_on_lines(const Vec3& p1, const Vec3& d1, const Vec3& p2, const Vec3& d2)
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

Line_T2 closest_points_on_lines(const Ray& r1, const Ray& r2)
{
	return closest_points_on_lines(r1.origin, r1.direction, r2.origin, r2.direction);
}

Line_T1 intersect_line_plane(const Vec3& pos, const Vec3& dir, const Vec3& normal, float d)
{
	Line_T1 ret;

	float divisor = dot(normal, dir);
	if (FAST_ABS(divisor) < COLLISION_EPSILON) {
		ret.t = 0.0f;
		ret.parallel = 1;
	} else {
		ret.t = (d - dot(normal, pos)) / divisor;
		ret.parallel = 0;
	}

	return ret;
}

Line_T1 intersect_line_plane(const Ray& line, const Plane& plane)
{
	return intersect_line_plane(line.origin, line.direction, plane.normal, plane.d);
}

struct Line_Sphere_T
{
	int num;
	float t[2];
};

Line_Sphere_T intersect_line_sphere_normalized(const Vec3& pos, const Vec3& dir, const Vec3& center, float radius)
{
	Line_Sphere_T ret;

	ASSERT_NEARLY_NORMALIZED(dir);

	Vec3 diff = center - pos;
	float proj = dot(dir, diff);
	float discriminant = proj*proj + length_squared(diff) + radius * radius;

	if (discriminant < 0.0f) {
		ret.num = 0;
	} else if (discriminant == 0.0f) {
		ret.num = 1;
		ret.t[0] = -proj;
	} else {
		ret.num = 2;
		float root = MSQRT(discriminant);
		ret.t[0] = -proj + root;
		ret.t[1] = -proj - root;
	}

	return ret;
}

Line_Sphere_T intersect_line_sphere(const Vec3& pos, const Vec3& dir, const Vec3& center, float radius)
{
	return intersect_line_sphere_normalized(pos, normalize(dir), center, radius);
}

Line_Sphere_T intersect_line_sphere(const Ray& ray, const Vec3& center, float radius)
{
	return intersect_line_sphere(ray.origin, ray.direction, center, radius);
}

