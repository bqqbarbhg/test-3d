
struct Debug_Line
{
	Vec3 a, b;
};

struct Debug_Point
{
	Vec3 p;
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

void debug_draw_line(Vec3 a, Vec3 b)
{
	if (g_debug_line_count >= Count(g_debug_lines))
		return;

	Debug_Line *line = &g_debug_lines[g_debug_line_count++];
	line->a = a;
	line->b = b;
}

void debug_draw_point(Vec3 p)
{
	if (g_debug_point_count >= Count(g_debug_points))
		return;

	Debug_Point *point = &g_debug_points[g_debug_point_count++];
	point->p = p;
}

void debug_draw_render()
{
	glBegin(GL_LINES);

	glColor3f(1.0f, 0.0f, 0.0f);
	for (U32 i = 0; i < g_debug_line_count; i++) {
		Debug_Line *line = &g_debug_lines[i];
		glVertex3fv((float*)&line->a);
		glVertex3fv((float*)&line->b);
	}

	glEnd();

	glBegin(GL_POINTS);

	glColor3f(1.0f, 0.0f, 0.0f);
	for (U32 i = 0; i < g_debug_line_count; i++) {
		Debug_Point *point = &g_debug_points[i];
		glVertex3fv((float*)&point->p);
	}

	glEnd();
}

