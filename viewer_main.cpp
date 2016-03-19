#include <GLFW/glfw3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

	if (!generate_shaders()) {
		return 1;
	}

	GL_Skinned_Mesh gl_mesh = { 0 };

	U32 bone_count;
	Mat44 bones[64];
	Mat44 bone_inv[64];

	{
		FILE *file = fopen("bin/out.bin", "rb");
		fseek(file, 0, SEEK_END);
		size_t size = ftell(file);
		fseek(file, 0, SEEK_SET);
		char *buffer = (char*)malloc(size);
		fread(buffer, size, 1, file);
		fclose(file);

		In_Stream ins = in_stream(buffer, size);

		stream_read(&ins, &bone_count, sizeof(U32));
		stream_read(&ins, bone_inv, sizeof(Mat44), bone_count);
		stream_read(&ins, bones, sizeof(Mat44), bone_count);
		read_skinned_mesh_to_gl(&ins, &gl_mesh);

		free(buffer);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	float yaw = 0.0f;
	float pitch = 0.0f;
	Vec3 camera_target = vec3(0.0f, 0.0f, 0.0f);

	while (!glfwWindowShouldClose(window)) {
		double time = glfwGetTime();

		int width, height;
		glfwGetWindowSize(window, &width, &height);

		Mat44 proj = mat44_perspective(1.0f, (float)width / (float)height, 0.01f, 100.0f);
		Mat44 projt = transpose(proj);

		Mat44 cam_mat = mat44_rotate_x(pitch) * mat44_rotate_y(yaw);
		Vec3 camera = vec3(0.0f, 0.0f, 15.0f) * cam_mat;
		Mat44 view = mat44_lookat(camera_target + camera, camera_target, vec3(0.0f, 1.0f, 0.0f));

		Mat44 world_to_screen = view * proj;
		Mat44 screen_to_world = inverse(world_to_screen);

		glEnable(GL_DEPTH_TEST);

		// Clearing the viewport
		glViewport(0, 0, width, height);
		glClearColor(0x64/255.0f, 0x95/255.0f, 0xED/255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		{
			Mat44 bone_trans[64];
			Mat44 vp = transpose(view * proj);

			for (U32 i = 0; i < gl_mesh.bone_count; i++) {
				const Mat44 &world = bones[i];
				bone_trans[i] = transpose(bone_inv[i] * world);
			}
			draw_skinned_mesh(&gl_mesh, vp, bone_trans);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glUseProgram(0);

		char *pixels = (char*)malloc(width * height * 3);
		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);

		for (int y = 0; y < height / 2; y++) {
			for (int x = 0; x < width; x++) {
				char *a = &pixels[(y * width + x) * 3];
				char *b = &pixels[((height - y - 1) * width + x) * 3];
				
				char t[3];
				t[0] = a[0]; t[1] = a[1]; t[2] = a[2];
				a[0] = b[0]; a[1] = b[1]; a[2] = b[2];
				b[0] = t[0]; b[1] = t[1]; b[2] = t[2];
			}
		}

		stbi_write_png("bin/cap.png", (int)width, (int)height, 3, pixels, 0);
		free(pixels);

		// Update GLFW
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

