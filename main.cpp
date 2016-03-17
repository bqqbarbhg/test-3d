#include <GLFW/glfw3.h>

#include "imgui_impl_glfw.h"

int main(int argc, char **argv)
{
	GLFWwindow *window;

	if (!glfwInit())
		return 1;

	glfwWindowHint(GLFW_SAMPLES, 4);

	window = glfwCreateWindow(640, 480, "Forest of sausages", NULL, NULL);
	if (!window) {
		fprintf(stderr, "Could not create window\n");
		glfwTerminate();
		return 2;
	}

	glfwMakeContextCurrent(window);

#ifdef _WIN32
	glewInit();
#endif

	ImGui_ImplGlfw_Init(window, true);

	if (!generate_shaders()) {
		return 1;
	}

	Model_File_Data *model = load_model_file(argv[1], 0);
	GL_Skinned_Mesh gl_mesh;

	int bone_mapping[64];
	Mat44 bone_inv[64];

	{
		Mesh *mesh = &model->meshes[0];

		for (U32 boneI = 0; boneI < mesh->bone_count; boneI++) {
			Bone *bone = &mesh->bones[boneI];

			for (U32 i = 0; i < model->node_count; i++) {
				if (!strcmp(bone->name, model->nodes[i].name)) {
					bone_mapping[boneI] = i;
					break;
				}
			}
			bone_inv[boneI] = bone->inv_bind_pose_transform;
		}

		if (!make_skinned_mesh(&gl_mesh, mesh)) {
			return 1;
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	Vec3 *temp_transform_buffer = (Vec3*)malloc(sizeof(Vec3) * 1024 * 32);

	Mat44 *world_transform = (Mat44*)malloc(sizeof(Mat44) * model->node_count);
	Mat44 *bone_transform = (Mat44*)malloc(sizeof(Mat44) * 256);

	float yaw = 0.0f;
	float pitch = 0.0f;
	Vec3 camera_target = vec3(0.0f, 0.0f, 0.0f);

	Vec2 prev_mouse_pos = vec2(0.0f, 0.0f);
	Editor_Mouse_State prev_editor_mouse = { 0 };

	Editor_Widget edit_widgets[64] = { 0 };
	U32 edit_nodes[64];
	U32 edit_object_count = 0;

	for (U32 i = 0; i < model->node_count; i++) {
		Node *node = &model->nodes[i];

		if (!strcmp(node->name, "Bone") ||
			!strcmp(node->name, "Bone.001") ||
			!strcmp(node->name, "Bone.002")) {

			edit_nodes[edit_object_count] = i;
			edit_widgets[edit_object_count].axis_pick_distance = 0.15f;
			edit_widgets[edit_object_count].do_flip = true;
			edit_object_count++;
		}
	}

	while (!glfwWindowShouldClose(window)) {
		double time = glfwGetTime();

		int width, height;
		glfwGetWindowSize(window, &width, &height);

		Mat44 proj = mat44_perspective(1.0f, (float)width / (float)height, 0.01f, 100.0f);
		Mat44 projt = transpose(proj);

		ImGui_ImplGlfw_NewFrame();
		debug_draw_reset();

		for (U32 i = 0; i < edit_object_count; i++) {
			editor_widget_reset(&edit_widgets[i]);
		}

		double x, y;
		glfwGetCursorPos(window, &x, &y);
		Vec2 mouse_pos = vec2((float)x, (float)y);
		Vec2 mouse_delta = mouse_pos - prev_mouse_pos;

		if (!ImGui::GetIO().WantCaptureMouse) {
			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)) {
				yaw -= mouse_delta.x * 0.01f;
				pitch -= mouse_delta.y * 0.01f;
			} else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE)) {
				Mat44 cam_mat = mat44_rotate_x(pitch) * mat44_rotate_y(yaw);
				Vec3 forward = vec3(0.0f, 0.0f, 1.0f) * cam_mat;
				Vec3 right = normalize(cross(vec3(0.0f, 1.0f, 0.0f), forward));
				Vec3 up = normalize(cross(right, forward));
				camera_target -= (mouse_delta.x * right + mouse_delta.y * up) * 0.01f;
			}
		}
		prev_mouse_pos = mouse_pos;

		if (pitch > 1.5f)
			pitch = 1.5f;
		if (pitch < -1.5f)
			pitch = -1.5f;

		Mat44 cam_mat = mat44_rotate_x(pitch) * mat44_rotate_y(yaw);
		Vec3 camera = vec3(0.0f, 0.0f, 15.0f) * cam_mat;
		Mat44 view = mat44_lookat(camera_target + camera, camera_target, vec3(0.0f, 1.0f, 0.0f));

		Mat44 world_to_screen = view * proj;
		Mat44 screen_to_world = inverse(world_to_screen);

		float mouse_relx = mouse_pos.x / (float)width * 2.0f - 1.0f;
		float mouse_rely = -(mouse_pos.y / (float)height * 2.0f - 1.0f);
		Vec4 mouse_near = vec4(mouse_relx, mouse_rely, 0.0f, 1.0f) * screen_to_world;
		Vec4 mouse_far = vec4(mouse_relx, mouse_rely, 1.0f, 1.0f) * screen_to_world;
		mouse_near *= 1.0f / mouse_near.w;
		mouse_far *= 1.0f / mouse_far.w;
		Ray mouse_ray = ray_to_point(
				vec3(mouse_near.x, mouse_near.y, mouse_near.z),
				vec3(mouse_far.x, mouse_far.y, mouse_far.z));

		Editor_Mouse_State editor_mouse;
		editor_mouse.world_ray = mouse_ray;
		editor_mouse.is_pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_RELEASE;

		bool widget_selected = false;
		for (U32 i = 0; i < edit_object_count; i++) {
			if (edit_widgets[i].selected_part) {
				edit_widgets[i].is_active = true;
				widget_selected = true;
				break;
			}
		}

		if (!ImGui::GetIO().WantCaptureMouse && !widget_selected) {

			float closest = FLT_MAX;
			int closest_i = -1;

			for (U32 i = 0; i < edit_object_count; i++) {
				float dist = editor_widget_pick(&edit_widgets[i], editor_mouse);
				if (dist < 0.0f) continue;

				if (dist < closest) {
					closest = dist;
					closest_i = (int)i;
				}
			}

			if (closest_i >= 0) {
				edit_widgets[closest_i].is_active = true;
			}
		}

		for (U32 i = 0; i < edit_object_count; i++) {
			if (!edit_widgets[i].is_active) continue;

			Mat44 xform;
			if (editor_widget_update(&edit_widgets[i], editor_mouse, prev_editor_mouse, &xform)) {
				Node *node = &model->nodes[edit_nodes[i]];
				const Mat44& parent = world_transform[node->parent - model->nodes];

				node->transform = world_transform[edit_nodes[i]] * xform * inverse(parent);
			}
		}

		prev_editor_mouse = editor_mouse;

		glColor3f(1.0f, 1.0f, 1.0f);

		Mat44 mat = mat44_rotate_x(sinf((float)time));

		for (U32 i = 0; i < model->node_count; i++) {
			Node *node = &model->nodes[i];

			if (node->parent) {
				world_transform[i] = node->transform * world_transform[node->parent - model->nodes];
			} else {
				world_transform[i] = node->transform;
			}
		}

		for (U32 i = 0; i < edit_object_count; i++) {
			editor_widget_set_mat44(&edit_widgets[i], world_transform[edit_nodes[i]]);
			editor_widget_set_camera_pos(&edit_widgets[i], camera);
		}

		glEnable(GL_DEPTH_TEST);

		// Clearing the viewport
		glViewport(0, 0, width, height);
		glClearColor(0x64/255.0f, 0x95/255.0f, 0xED/255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		Mat44 viewt = transpose(view);

		// Temp matrix stuffs
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(projt.data);

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(viewt.data);

		static bool do_wireframe = false;
		ImGui::Checkbox("Wireframe", &do_wireframe);
		if (do_wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		{
			Mat44 bone_trans[64];
			Mat44 vp = transpose(view * proj);

			for (U32 i = 0; i < gl_mesh.bone_count; i++) {
				const Mat44 &world = world_transform[bone_mapping[i]];
				bone_trans[i] = transpose(bone_inv[i] * world);
			}
			draw_skinned_mesh(&gl_mesh, vp, bone_trans);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glUseProgram(0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glClear(GL_DEPTH_BUFFER_BIT);

		for (U32 i = 0; i < edit_object_count; i++) {
			editor_widget_draw(&edit_widgets[i]);
		}

		static bool do_debug_draw = true;
		ImGui::Checkbox("Debug debug lines", &do_debug_draw);
		if (do_debug_draw) {
			debug_draw_render();
		}

		// Render GUI
		ImGui::Render();

		// Update GLFW
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	free_model_file(model);

	ImGui_ImplGlfw_Shutdown();
	glfwTerminate();
	return 0;
}

