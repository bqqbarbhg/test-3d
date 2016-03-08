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

	while (!glfwWindowShouldClose(window)) {
		double time = glfwGetTime();

		ImGui_ImplGlfw_NewFrame();

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		for (U32 i = 0; i < model->node_count; i++) {
			Node *node = &model->nodes[i];

			if (node->parent) {
				world_transform[i] = node->transform * world_transform[node->parent - model->nodes];
			} else {
				world_transform[i] = node->transform;
			}

			if (!strcmp(node->name, "Bone.001")) {
				world_transform[i] = mat44_rotate_x(-sinf((float)time)) * world_transform[i];
			} else if (!strcmp(node->name, "Bone.002")) {
				world_transform[i] = mat44_rotate_x(sinf((float)time)) * world_transform[i];
			}
		}


		// Clearing the viewport
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(0x64/255.0f, 0x95/255.0f, 0xED/255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Temp matrix stuffs
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f, (float)width / (float)height, 0.01f, 100.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(15.0f, 15.0f, 15.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

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

				glBegin(GL_TRIANGLES);
				U32 index_count = mesh->index_count;
				for (U32 i = 0; i < index_count; i++) {
					Vec3 pos = temp_transform_buffer[mesh->indices[i]];
					glVertex3f(pos.x, pos.y, pos.z);
				}

				glEnd();
			}
		}

		// Render GUI
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

