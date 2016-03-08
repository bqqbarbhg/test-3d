

#include <GLFW/glfw3.h>

#include <imgui.h>
#include "imgui_impl_glfw.h"

float g_time;

struct Node_Transform
{
	aiString name;
	aiMatrix4x4 transform;
};

Node_Transform node_transforms[128];

aiVector3D *get_transform_verts(size_t count)
{
	static size_t num_transform_verts = 0;
	static aiVector3D *transform_verts = 0;

	if (num_transform_verts < count) {
		num_transform_verts = count;
		transform_verts = (aiVector3D*)realloc(transform_verts, sizeof(aiVector3D) * count);
	}
	return transform_verts;
}

aiMatrix4x4 world_transform(const aiNode *node)
{
	aiMatrix4x4 transform;
	if (node->mParent) {
		transform = world_transform(node->mParent) * node->mTransformation;
	} else {
		transform = node->mTransformation;
	}

	for (int i = 0; i < 128; i++) {
		if (node_transforms[i].name == node->mName) {
			transform = transform * node_transforms[i].transform;
			break;
		}
	}

	return transform;
}

void draw_node(const aiScene *scene, const aiNode *node, const aiMatrix4x4& parent = aiMatrix4x4())
{
	aiMatrix4x4 transform = node->mTransformation * parent;

	for (unsigned i = 0; i < node->mNumChildren; i++) {
		draw_node(scene, node->mChildren[i], transform);
	}

	for (unsigned meshI = 0; meshI < node->mNumMeshes; meshI++) {
		aiMesh *mesh = scene->mMeshes[node->mMeshes[meshI]];

		aiVector3D *transform_verts = get_transform_verts(mesh->mNumVertices);
		memset(transform_verts, 0, sizeof(aiVector3D) * mesh->mNumVertices);

		for (unsigned boneI = 0; boneI < mesh->mNumBones; boneI++) {
			aiBone *bone = mesh->mBones[boneI];
			aiNode *node = scene->mRootNode->FindNode(bone->mName);
			if (!node)
				continue;

			for (unsigned weightI = 0; weightI < bone->mNumWeights; weightI++) {
				aiVertexWeight weight = bone->mWeights[weightI];

				aiVector3D vert = mesh->mVertices[weight.mVertexId];
				vert *= bone->mOffsetMatrix;
				vert *= world_transform(node);
				vert *= weight.mWeight;

				transform_verts[weight.mVertexId] += vert;
			}
		}

		glBegin(GL_TRIANGLES);

		unsigned faceCount = mesh->mNumFaces;
		for (unsigned faceI = 0; faceI < faceCount; faceI++) {
			aiFace face = mesh->mFaces[faceI];

			for (unsigned vertexI = 0; vertexI < 3; vertexI++) {
				unsigned index = face.mIndices[vertexI];
				aiVector3D pos = transform_verts[index];
				// pos *= transform;
				glVertex3f(pos.x, pos.y, pos.z);
			}
		}

		glEnd();
	}
}

void gui_nodes(const Node *node)
{
	if (ImGui::TreeNode(node, "%s", node->name)) {

		for (unsigned i = 0; i < node->child_count; i++) {
			gui_nodes(node->children[i]);
		}

		ImGui::TreePop();
	}
}

int main(int argc, char **argv)
{
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(640, 480, "Forest of sausages", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    Model_File_Data *data = load_model_file(argv[1], 0);

	ImGui_ImplGlfw_Init(window, true);

	const aiScene *scene = aiImportFile(argv[1], aiProcess_Triangulate);
	if (!scene) {
		printf("Import error: %s\n", aiGetErrorString());
		return 1;
	}

    while (!glfwWindowShouldClose(window))
    {
		ImGui_ImplGlfw_NewFrame();

		static float time_scale = 1.0f;
		ImGui::SliderFloat("Time scale", &time_scale, 0.0f, 5.0f);

		ImGui::Begin("Scene");
		gui_nodes(data->root_node);
		ImGui::End();

		aiMatrix4x4 rotation;
		aiMatrix4x4::RotationZ(sinf(g_time) * 0.5f, rotation);

		aiMatrix4x4 rotationY;
		aiMatrix4x4::RotationY(g_time * 0.5f, rotationY);

		node_transforms[0].name.Set("Bone");
		node_transforms[0].transform = rotationY;
		node_transforms[1].name.Set("Bone.001");
		node_transforms[1].transform = rotation;
		node_transforms[2].name.Set("Bone.002");
		node_transforms[2].transform = rotation;

		int width, height;
		glfwGetWindowSize(window, &width, &height);

		glViewport(0, 0, width, height);

		glClearColor(0x64/255.0f, 0x95/255.0f, 0xED/255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0f, (float)width / (float)height, 0.01f, 100.0f);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(15.0f, 15.0f, 15.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		draw_node(scene, scene->mRootNode);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		ImGui::Render();

        glfwSwapBuffers(window);
        glfwPollEvents();

		g_time += 0.016f * time_scale;
    }

	ImGui_ImplGlfw_Shutdown();

	aiReleaseImport(scene);

    glfwTerminate();
    return 0;
}

