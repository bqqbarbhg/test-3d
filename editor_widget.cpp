
enum Editor_Widget_Part
{
	Editor_Widget_Part_None,
	Editor_Widget_Part_X_Axis,
	Editor_Widget_Part_Y_Axis,
	Editor_Widget_Part_Z_Axis,
	Editor_Widget_Part_XY_Plane,
	Editor_Widget_Part_XZ_Plane,
	Editor_Widget_Part_YZ_Plane,
	Editor_Widget_Part_XY_Ring,
	Editor_Widget_Part_XZ_Ring,
	Editor_Widget_Part_YZ_Ring,
};

struct Editor_Widget
{
	Vec3 position;
	Vec3 axes[3];
	bool flip[3];

	float axis_pick_distance;

	bool do_flip;
	bool is_active;

	Editor_Widget_Part selected_part;
	Editor_Widget_Part hovered_part;
};


struct Editor_Widget_Plane
{
	int a;
	int b;
	int normal;
};
const Editor_Widget_Plane editor_widget_planes[] = {
	{ 0, 1, 2 },
	{ 0, 2, 1 },
	{ 1, 2, 0 },
};

void editor_widget_set_mat44(Editor_Widget *w, const Mat44& m)
{
	w->position = vec3(m._14, m._24, m._34);
	w->axes[0] = vec3(m._11, m._21, m._31);
	w->axes[1] = vec3(m._12, m._22, m._32);
	w->axes[2] = vec3(m._13, m._23, m._33);
}

struct Editor_Mouse_State
{
	Ray world_ray;
	bool is_pressed;
};

Ray editor_widget_axis(const Editor_Widget *w, int axis)
{
	float sign = w->flip[axis] ? -1.0f : 1.0f;
	return ray_to_dir(w->position, w->axes[axis] * 2.0f * sign);
}

void editor_widget_reset(Editor_Widget *w)
{
	w->hovered_part = Editor_Widget_Part_None;
	w->is_active = false;
}

float editor_widget_pick(Editor_Widget *w, Editor_Mouse_State mouse)
{
	// Check for axis intersection
	{
		float closest = w->axis_pick_distance;
		float closest_t = 0.0f;
		int closest_axis = -1;

		for (int i = 0; i < 3; i++) {
			Ray axis = editor_widget_axis(w, i);
			Line_T2 ts = closest_points_on_lines(axis, mouse.world_ray);
			if (ts.parallel || ts.t2 < 0.0f)
				continue;

			if (ts.t1 < 0.0f || ts.t1 > 1.0f)
				continue;

			float distance = length(ray_at(axis, ts.t1) - ray_at(mouse.world_ray, ts.t2));
			if (distance < closest) {
				closest = distance;
				closest_t = ts.t2;
				closest_axis = i;
			}
		}

		if (closest_axis >= 0) {
			w->hovered_part = (Editor_Widget_Part)(Editor_Widget_Part_X_Axis + closest_axis);
			return closest_t;
		}
	}

	// Check for plane quad intersection
	{
		float closest = FLT_MAX;
		int closest_plane = -1;
		for (int i = 0; i < Count(editor_widget_planes); i++) {
			Editor_Widget_Plane plane = editor_widget_planes[i];
			Plane p = plane_from_point_normal(w->position, w->axes[plane.normal]);
			Line_T1 ts = intersect_line_plane(mouse.world_ray, p);
			if (ts.parallel || ts.t < 0.0f)
				continue;

			Vec3 point = ray_at(mouse.world_ray, ts.t);
			Vec3 dir = point - w->position;

			Vec3 a = editor_widget_axis(w, plane.a).direction;
			Vec3 b = editor_widget_axis(w, plane.b).direction;

			float at = dot(dir, normalize(a));
			float bt = dot(dir, normalize(b));

			if (at < 0.0f || at > 0.5f * length(a)) continue;
			if (bt < 0.0f || bt > 0.5f * length(a)) continue;

			if (ts.t < closest) {
				closest = ts.t;
				closest_plane = i;
			}
		}

		if (closest_plane >= 0) {
			w->hovered_part = (Editor_Widget_Part)(Editor_Widget_Part_XY_Plane + closest_plane);
			return closest;
		}
	}

	// Check for plane ring intersection
	{
		float closest = FLT_MAX;
		int closest_plane = -1;
		for (int i = 0; i < Count(editor_widget_planes); i++) {
			Editor_Widget_Plane plane = editor_widget_planes[i];
			Plane p = plane_from_point_normal(w->position, w->axes[plane.normal]);
			Line_T1 ts = intersect_line_plane(mouse.world_ray, p);
			if (ts.parallel || ts.t < 0.0f)
				continue;

			Vec3 point = ray_at(mouse.world_ray, ts.t);
			float dist = length(point - w->position);

			if (dist < 1.95f || dist > 2.15f)
				continue;

			if (ts.t < closest) {
				closest = ts.t;
				closest_plane = i;
			}
		}

		if (closest_plane >= 0) {
			w->hovered_part = (Editor_Widget_Part)(Editor_Widget_Part_XY_Ring + closest_plane);
			return closest;
		}
	}

	w->hovered_part = Editor_Widget_Part_None;
	return -1.0f;
}

bool editor_widget_update(Editor_Widget *w, Editor_Mouse_State mouse, Editor_Mouse_State prev_mouse, Mat44 *transform)
{
	if (!mouse.is_pressed) {
		w->selected_part = Editor_Widget_Part_None;
		return false;
	}

	if (!prev_mouse.is_pressed) {
		w->selected_part = w->hovered_part;
		return false;
	}

	Editor_Widget_Part part = w->selected_part;
	if (!part)
		return false;

	if (part >= Editor_Widget_Part_X_Axis && part <= Editor_Widget_Part_Z_Axis) {
		Ray axis = editor_widget_axis(w, part - Editor_Widget_Part_X_Axis);

		Line_T2 prev_ts = closest_points_on_lines(axis, prev_mouse.world_ray);
		Line_T2 cur_ts = closest_points_on_lines(axis, mouse.world_ray);

		if (prev_ts.parallel || cur_ts.parallel || prev_ts.t2 < 0.0f || cur_ts.t2 < 0.0f)
			return false;

		Vec3 prev_pos = ray_at(axis, prev_ts.t1);
		Vec3 cur_pos = ray_at(axis, cur_ts.t1);

		*transform = mat44_translate(cur_pos - prev_pos);
		return true;
	}

	if (part >= Editor_Widget_Part_XY_Plane && part <= Editor_Widget_Part_YZ_Plane) {
		Editor_Widget_Plane plane = editor_widget_planes[part - Editor_Widget_Part_XY_Plane];
		Plane p = plane_from_point_normal(w->position, w->axes[plane.normal]);

		Line_T1 prev_ts = intersect_line_plane(prev_mouse.world_ray, p);
		Line_T1 cur_ts = intersect_line_plane(mouse.world_ray, p);

		if (prev_ts.parallel || cur_ts.parallel || prev_ts.t < 0.0f || cur_ts.t < 0.0f)
			return false;

		Vec3 prev_pos = ray_at(prev_mouse.world_ray, prev_ts.t);
		Vec3 cur_pos = ray_at(mouse.world_ray, cur_ts.t);

		*transform = mat44_translate(cur_pos - prev_pos);
		return true;
	}

	if (part >= Editor_Widget_Part_XY_Ring && part <= Editor_Widget_Part_YZ_Ring) {
		Editor_Widget_Plane plane = editor_widget_planes[part - Editor_Widget_Part_XY_Ring];
		Plane p = plane_from_point_normal(w->position, w->axes[plane.normal]);

		Line_T1 prev_ts = intersect_line_plane(prev_mouse.world_ray, p);
		Line_T1 cur_ts = intersect_line_plane(mouse.world_ray, p);

		if (prev_ts.parallel || cur_ts.parallel || prev_ts.t < 0.0f || cur_ts.t < 0.0f)
			return false;

		Vec3 prev_pos = ray_at(prev_mouse.world_ray, prev_ts.t) - w->position;
		Vec3 cur_pos = ray_at(mouse.world_ray, cur_ts.t) - w->position;

		Vec3 norm = normalize(w->axes[plane.normal]);
		Vec3 right;
		if (fabs(dot(norm, vec3(0.0f, 1.0f, 0.0f))) > 0.9f) {
			right = normalize(cross(norm, vec3(1.0f, 0.0f, 0.0f)));
		} else {
			right = normalize(cross(norm, vec3(0.0f, 1.0f, 0.0f)));
		}
		Vec3 up = normalize(cross(right, norm));

		Vec2 prev_flat = vec2(dot(prev_pos, right), dot(prev_pos, up));
		Vec2 cur_flat = vec2(dot(cur_pos, right), dot(cur_pos, up));

		float angle = atan2f(cur_flat.y, cur_flat.x) - atan2f(prev_flat.y, prev_flat.x);

		*transform = mat44_translate(-w->position)
				* mat44_rotate_axis(w->axes[plane.normal], -angle)
				* mat44_translate(w->position);
		return true;
	}

	return false;
}

void editor_widget_set_camera_pos(Editor_Widget *w, const Vec3& pos)
{
	if (w->do_flip) {
		Vec3 diff = pos - w->position;
		for (int i = 0; i < 3; i++) {
			w->flip[i] = dot(diff, w->axes[i]) < 0.0f;
		}
	} else {
		for (int i = 0; i < 3; i++) {
			w->flip[i] = false;
		}
	}
}

void editor_widget_draw(Editor_Widget *w)
{
	static const Vec3 axis_color[] = {
		{ 0.9f, 0.0f, 0.0f },
		{ 0.0f, 0.9f, 0.0f },
		{ 0.0f, 0.0f, 0.9f },
	};
	static const Vec3 axis_color_selected[] = {
		{ 1.0f, 0.3f, 0.3f },
		{ 0.3f, 1.0f, 0.3f },
		{ 0.3f, 0.3f, 1.0f },
	};

	Editor_Widget_Part highlight_part = Editor_Widget_Part_None;
	if (w->selected_part)
		highlight_part = w->selected_part;
	else if (w->hovered_part)
		highlight_part = w->hovered_part;

	glEnableClientState(GL_VERTEX_ARRAY);

	// Draw arrows
	for (int i = 0; i < 3; i++) {
		Ray axis = editor_widget_axis(w, i);

		const int circle_segments = 8;
		Vec3 vertices[circle_segments * 3 + 2];
		U16 indices[circle_segments * 18];

		Vec3 norm = normalize(axis.direction);
		Vec3 right;
		if (fabs(dot(norm, vec3(0.0f, 1.0f, 0.0f))) > 0.9f) {
			right = normalize(cross(norm, vec3(1.0f, 0.0f, 0.0f)));
		} else {
			right = normalize(cross(norm, vec3(0.0f, 1.0f, 0.0f)));
		}
		Vec3 up = normalize(cross(right, norm));

		const float tail_radius = 0.05f;
		const float head_radius = 0.1f;
		const float head_lenth = 0.4f;

		float tail_length = length(axis.direction) - head_lenth;
		Vec3 head_start = axis.origin + norm * MMAX(tail_length, 0.0f);

		vertices[0] = axis.origin;
		vertices[1] = axis.origin + axis.direction;

		U16 *iptr = indices;
		for (int segment = 0; segment < circle_segments; segment++) {
			float angle = (float)segment / (float)circle_segments * FLT_PI * 2.0f;
			Vec3 dir = right * cosf(angle) + up * sinf(angle);

			vertices[2 + segment * 3 + 0] = axis.origin + dir * tail_radius;
			vertices[2 + segment * 3 + 1] = head_start + dir * tail_radius;
			vertices[2 + segment * 3 + 2] = head_start + dir * head_radius;

			unsigned next_segment = (segment + 1) % circle_segments;

			U16 a = (U16)(2 + segment * 3);
			U16 b = (U16)(2 + next_segment * 3);

			*iptr++ =   0; *iptr++ = a+0; *iptr++ = b+0;
			*iptr++ = a+0; *iptr++ = b+0; *iptr++ = a+1;
			*iptr++ = b+0; *iptr++ = a+1; *iptr++ = b+1;
			*iptr++ = a+1; *iptr++ = b+1; *iptr++ = b+2;
			*iptr++ = b+1; *iptr++ = a+2; *iptr++ = b+2;
			*iptr++ = a+2; *iptr++ = b+2; *iptr++ =   1;
		}
		
		if (w->is_active && highlight_part == Editor_Widget_Part_X_Axis + i) {
			glColor3fv((GLfloat*)&axis_color_selected[i]);
		} else {
			glColor3fv((GLfloat*)&axis_color[i]);
		}
		
		glVertexPointer(3, GL_FLOAT, 0, vertices);
		glDrawElements(GL_TRIANGLES, Count(indices), GL_UNSIGNED_SHORT, indices);
	}

	// Draw plane quads
	for (int i = 0; i < Count(editor_widget_planes); i++) {
		Editor_Widget_Plane plane = editor_widget_planes[i];
		Vec3 a = editor_widget_axis(w, plane.a).direction * 0.5f;
		Vec3 b = editor_widget_axis(w, plane.b).direction * 0.5f;

		const float frame_thickness = 0.1f;
		const float frame_start = 1.0f - frame_thickness;

		Vec3 vertices[6];
		vertices[0] = w->position + frame_start * a;
		vertices[1] = w->position + a;
		vertices[2] = w->position + frame_start * (a + b);
		vertices[3] = w->position + a + b;
		vertices[4] = w->position + frame_start * b;
		vertices[5] = w->position + b;

		const U16 indices[12] = {
			0, 1, 2, 2, 1, 3,
			2, 3, 4, 4, 3, 5,
		};

		Vec3 col;
		if (w->is_active && highlight_part == Editor_Widget_Part_XY_Plane + i) {
			col = vec3(1.0f, 0.2f, 0.2f);
		} else {
			col = vec3(0.8f, 0.0f, 0.0f);
		}
		glColor3fv((GLfloat*)&col);

		glVertexPointer(3, GL_FLOAT, 0, vertices);
		glDrawElements(GL_TRIANGLES, Count(indices), GL_UNSIGNED_SHORT, indices);
	}

	// Draw rings
	for (int i = 0; i < Count(editor_widget_planes); i++) {
		Editor_Widget_Plane plane = editor_widget_planes[i];
		Vec3 a = normalize(editor_widget_axis(w, plane.a).direction);
		Vec3 b = normalize(editor_widget_axis(w, plane.b).direction);

		const int ring_segments = 64;
		Vec3 vertices[(ring_segments + 1) * 2];

		float inner_radius = 2.0f;
		float outer_radius = 2.1f;

		for (int segment = 0; segment < ring_segments + 1; segment++) {
			float angle = (float)segment / (float)ring_segments * FLT_PI * 2.0f;
			Vec3 dir = a * cosf(angle) + b * sinf(angle);

			vertices[segment * 2 + 0] = w->position + dir * inner_radius;
			vertices[segment * 2 + 1] = w->position + dir * outer_radius;
		}

		if (w->is_active && highlight_part == Editor_Widget_Part_XY_Ring + i) {
			glColor3fv((GLfloat*)&axis_color_selected[plane.normal]);
		} else {
			glColor3fv((GLfloat*)&axis_color[plane.normal]);
		}

		glVertexPointer(3, GL_FLOAT, 0, vertices);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, Count(vertices));
	}

	glVertexPointer(3, GL_FLOAT, 0, 0);
	glDisableClientState(GL_VERTEX_ARRAY);
}

