
struct Debug_Line
{
	Vec3 a, b;
	Vec3 color;
};

struct Debug_Point
{
	Vec3 p;
	Vec3 color;
};

Debug_Line g_debug_lines[2048];
Debug_Point g_debug_points[2048];

U32 g_debug_line_count;
U32 g_debug_point_count;

void debug_draw_reset()
{
	g_debug_line_count = 0;
	g_debug_point_count = 0;
}

void debug_draw_line(Vec3 a, Vec3 b, Vec3 color=vec3(1.0f, 0.0f, 0.0f))
{
	if (g_debug_line_count >= Count(g_debug_lines))
		return;

	Debug_Line *line = &g_debug_lines[g_debug_line_count++];
	line->a = a;
	line->b = b;
	line->color = color;
}

void debug_draw_line(Ray r, Vec3 color=vec3(1.0f, 0.0f, 0.0f))
{
	debug_draw_line(r.origin, r.origin + r.direction, color);
}

void debug_draw_point(Vec3 p, Vec3 color=vec3(1.0f, 0.0f, 0.0f))
{
	if (g_debug_point_count >= Count(g_debug_points))
		return;

	Debug_Point *point = &g_debug_points[g_debug_point_count++];
	point->p = p;
	point->color = color;
}

void debug_draw_render()
{
	glBegin(GL_LINES);

	for (U32 i = 0; i < g_debug_line_count; i++) {
		Debug_Line *line = &g_debug_lines[i];
		glColor3fv((float*)&line->color);
		glVertex3fv((float*)&line->a);
		glVertex3fv((float*)&line->b);
	}

	glEnd();

	glPointSize(5.0f);
	glBegin(GL_POINTS);

	for (U32 i = 0; i < g_debug_point_count; i++) {
		Debug_Point *point = &g_debug_points[i];
		glColor3fv((float*)&point->color);
		glVertex3fv((float*)&point->p);
	}

	glEnd();
}

