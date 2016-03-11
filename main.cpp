#include <GLFW/glfw3.h>

#include <imgui.h>
#include "imgui_impl_glfw.h"

int main(int argc, char **argv)
{
	GLFWwindow *window;

	if (!glfwInit())
		return 1;

	window = glfwCreateWindow(640, 480, "Forest of sausages", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return 2;
	}

	glfwMakeContextCurrent(window);

	ImGui_ImplGlfw_Init(window, true);

	Model_File_Data *model = load_model_file(argv[1], 0);

	Vec3 *temp_transform_buffer = (Vec3*)malloc(sizeof(Vec3) * 1024 * 32);

	Mat44 *world_transform = (Mat44*)malloc(sizeof(Mat44) * model->node_count);
	Mat44 *bone_transform = (Mat44*)malloc(sizeof(Mat44) * 256);

	float yaw = 0.0f;
	float pitch = 0.0f;
	Vec3 camera_target = vec3(0.0f, 0.0f, 0.0f);

	Vec2 prev_mouse_pos = vec2(0.0f, 0.0f);

	while (!glfwWindowShouldClose(window)) {
		double time = glfwGetTime();

		ImGui_ImplGlfw_NewFrame();
		debug_draw_reset();

		if (!ImGui::GetIO().WantCaptureMouse) {

			double x, y;
			glfwGetCursorPos(window, &x, &y);
			Vec2 mouse_pos = vec2((float)x, (float)y);
			Vec2 mouse_delta = mouse_pos - prev_mouse_pos;

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

			prev_mouse_pos = mouse_pos;
		}

		glColor3f(1.0f, 1.0f, 1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		Mat44 mat = mat44_rotate_x(sinf((float)time));

		for (U32 i = 0; i < model->node_count; i++) {
			Node *node = &model->nodes[i];

			if (node->parent) {
				world_transform[i] = node->transform * world_transform[node->parent - model->nodes];
			} else {
				world_transform[i] = node->transform;
			}

			if (!strcmp(node->name, "Bone.001")) {
				world_transform[i] = mat * world_transform[i];
			} else if (!strcmp(node->name, "Bone.002")) {
				world_transform[i] = inverse(mat) * world_transform[i];
			}
		}

		// Clearing the viewport
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(0x64/255.0f, 0x95/255.0f, 0xED/255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		Mat44 proj = mat44_perspective(1.0f, (float)width / (float)height, 0.01f, 100.0f);
		Mat44 projt = transpose(proj);

		if (pitch > 1.5f)
			pitch = 1.5f;
		if (pitch < -1.5f)
			pitch = -1.5f;

		Mat44 cam_mat = mat44_rotate_x(pitch) * mat44_rotate_y(yaw);
		Vec3 camera = vec3(0.0f, 0.0f, 10.0f) * cam_mat;
		Mat44 view = mat44_lookat(camera_target + camera, camera_target, vec3(0.0f, 1.0f, 0.0f));

		ImGui::Begin("View matrix");
		ImGui::Text("%6.2f %6.2f %6.2f %6.2f", view._11, view._12, view._13, view._14);
		ImGui::Text("%6.2f %6.2f %6.2f %6.2f", view._21, view._22, view._23, view._24);
		ImGui::Text("%6.2f %6.2f %6.2f %6.2f", view._31, view._32, view._33, view._34);
		ImGui::Text("%6.2f %6.2f %6.2f %6.2f", view._41, view._42, view._43, view._44);
		ImGui::End();

		Mat44 viewt = transpose(view);

		// Temp matrix stuffs
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(projt.data);

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(viewt.data);

		U32 node_count = model->node_count;
		for (U32 nodeI = 0; nodeI < node_count; nodeI++) {
			Node *node = &model->nodes[nodeI];

			const Mat44 &transform = world_transform[nodeI];

			U32 mesh_count = node->mesh_count;
			for (U32 meshI = 0; meshI < mesh_count; meshI++) {
				Mesh *mesh = node->meshes[meshI];

				U32 bone_count = mesh->bone_count;
				for (U32 boneI = 0; boneI < bone_count; boneI++) {
					Bone *bone = &mesh->bones[boneI];
					Mat44 *bonetrans = 0;

					for (U32 i = 0; i < node_count; i++) {
						if (!strcmp(bone->name, model->nodes[i].name)) {
							bonetrans = &world_transform[i];
							break;
						}
					}

					assert(bonetrans);

					bone_transform[boneI] = bone->inv_bind_pose_transform * *bonetrans;
				}

				U32 vertex_count = mesh->vertex_count;
				U32 bones_per_vertex = mesh->bones_per_vertex;
				for (U32 vertexI = 0; vertexI < vertex_count; vertexI++) {
					U8 *bone_indices = &mesh->bone_indices[vertexI * bones_per_vertex];
					float *bone_weights = &mesh->bone_weights[vertexI * bones_per_vertex];

					Vec3 vertex = mesh->positions[vertexI];
					Vec3 result = vec3_zero;

					for (U32 i = 0; i < bones_per_vertex; i++) {
						result += (vertex * bone_transform[bone_indices[i]]) * bone_weights[i];
					}

					temp_transform_buffer[vertexI] = result;
				}

				static bool do_mesh_draw = false;
				ImGui::Checkbox("Draw mesh", &do_mesh_draw);

				if (do_mesh_draw) {
					glBegin(GL_TRIANGLES);
					U32 index_count = mesh->index_count;
					for (U32 i = 0; i < index_count; i++) {
						Vec3 pos = temp_transform_buffer[mesh->indices[i]];
						glVertex3f(pos.x, pos.y, pos.z);
					}
					glEnd();
				}
			}
		}

		debug_draw_line(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 5.0f, 0.0f));

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

