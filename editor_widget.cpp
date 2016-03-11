
enum Editor_Widget_Part
{
	Editor_Widget_Part_None,
	Editor_Widget_Part_X_Axis,
	Editor_Widget_Part_Y_Axis,
	Editor_Widget_Part_Z_Axis,
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

void editor_widget_set_mat44(Editor_Widget *w, const Mat44& m)
{
	w->position = vec3(m._14, m._24, m._34);
	w->axes[0] = vec3(m._11, m._12, m._13);
	w->axes[1] = vec3(m._21, m._22, m._23);
	w->axes[2] = vec3(m._31, m._32, m._33);
}

struct Editor_Mouse_State
{
	Ray world_ray;
	bool is_pressed;
};

Ray editor_widget_axis(const Editor_Widget *w, int axis)
{
	float sign = w->flip[axis] ? -1.0f : 1.0f;
	return ray_to_dir(w->position, w->axes[axis] * 1.0f * sign);
}

void editor_widget_reset(Editor_Widget *w)
{
	w->hovered_part = Editor_Widget_Part_None;
	w->is_active = false;
}

float editor_widget_pick(Editor_Widget *w, Editor_Mouse_State mouse)
{
	float closest = w->axis_pick_distance;
	float closest_t = 0.0f;
	int closest_axis = -1;

	for (int i = 0; i < 3; i++) {
		Ray axis = editor_widget_axis(w, i);
		Line_T2 ts = closest_points_on_lines(axis, mouse.world_ray);
		if (ts.parallel)
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

	if (closest_axis < 0) {
		w->hovered_part = Editor_Widget_Part_None;
		return -1.0f;
	}

	w->hovered_part = (Editor_Widget_Part)(Editor_Widget_Part_X_Axis + closest_axis);
	return closest_t;
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

		Vec3 prev_pos = ray_at(axis, prev_ts.t1);
		Vec3 cur_pos = ray_at(axis, cur_ts.t1);

		*transform = mat44_translate(cur_pos - prev_pos);
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
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f },
	};
	static const Vec3 axis_color_selected[] = {
		{ 1.0f, 0.5f, 0.5f },
		{ 0.5f, 1.0f, 0.5f },
		{ 0.3f, 0.3f, 1.0f },
	};

	Editor_Widget_Part highlight_part = Editor_Widget_Part_None;
	if (w->selected_part)
		highlight_part = w->selected_part;
	else if (w->hovered_part)
		highlight_part = w->hovered_part;

	for (int i = 0; i < 3; i++) {
		Ray axis = editor_widget_axis(w, i);

		if (w->is_active && highlight_part == Editor_Widget_Part_X_Axis + i) {
			debug_draw_line(axis, axis_color_selected[i]);
		} else {
			debug_draw_line(axis, axis_color[i]);
		}
	}
}

