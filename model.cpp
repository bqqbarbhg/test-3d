#include "model.h"
#include <assimp/cimport.h>
#include <assimp/Importer.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


const static Model_File_Settings default_model_file_settings = {};

class Assimp_Progress_Handler : public Assimp::ProgressHandler
{
	model_progress_callback callback;

public:
	Assimp_Progress_Handler()
		: callback(0)
	{
	}

	Assimp_Progress_Handler(model_progress_callback callback)
		: callback(callback)
	{
	}

	virtual bool Update(float percentage)
	{
		if (callback)
			callback(percentage);
		return true;
	}
};

static Mat44 translate_matrix(const aiMatrix4x4& mat)
{
	Mat44 ret;

	ret._11 = mat.a1;
	ret._12 = mat.a2;
	ret._13 = mat.a3;
	ret._14 = mat.a4;

	ret._21 = mat.b1;
	ret._22 = mat.b2;
	ret._23 = mat.b3;
	ret._24 = mat.b4;

	ret._31 = mat.c1;
	ret._32 = mat.c2;
	ret._33 = mat.c3;
	ret._34 = mat.c4;

	ret._41 = mat.d1;
	ret._42 = mat.d2;
	ret._43 = mat.d3;
	ret._44 = mat.d4;

	return ret;
}

static Node* create_nodes_recursive(Temp_Allocator *t, Model_File_Data *data, aiNode *ai_node)
{
	Node *node = &data->nodes[data->node_count++];

	TEMP_COPY_STR(t, node->name, ai_node->mName.data);
	node->transform = translate_matrix(ai_node->mTransformation);

	U32 child_count = ai_node->mNumChildren;
	node->child_count = child_count;

	U32 mesh_count = ai_node->mNumMeshes;
	node->mesh_count = mesh_count;

	TEMP_ALLOC_N(t, node->meshes, mesh_count);
	for (U32 meshI = 0; meshI < mesh_count; meshI++) {
		TEMP_POINTER_SET(t, node->meshes[meshI], &data->meshes[meshI]);
	}

	TEMP_ALLOC_N(t, node->children, child_count);

	for (U32 i = 0; i < child_count; i++) {
		Node *child = create_nodes_recursive(t, data, ai_node->mChildren[i]);

		TEMP_POINTER_SET(t, child->parent, node);
		TEMP_POINTER_SET(t, node->children[i], child);
	}

	return node;
}

Model_File_Data *load_model_file(const char *file, const Model_File_Settings *settings)
{
	if (!settings)
		settings = &default_model_file_settings;

	Assimp::Importer importer;
	importer.SetProgressHandler(new Assimp_Progress_Handler(settings->progress_callback));

	const aiScene *scene = importer.ReadFile(file, aiProcess_Triangulate);

	Temp_Allocator backup_alloc = { 0 }, *t = &backup_alloc;
	if (settings->temp_allocator)
		t = settings->temp_allocator;

	Model_File_Data *temp_data;
	TEMP_ALLOC_N(t, temp_data, 1);

	const U32 num_meshes = scene->mNumMeshes;
	temp_data->mesh_count = num_meshes;

	TEMP_ALLOC_N(t, temp_data->meshes, num_meshes);

	for (U32 meshI = 0; meshI < num_meshes; meshI++) {
		const aiMesh *ai_mesh = scene->mMeshes[meshI];
		Mesh *mesh = &temp_data->meshes[meshI];

		const U32 vertex_count = ai_mesh->mNumVertices;
		mesh->vertex_count = vertex_count;

		TEMP_COPY_STR(t, mesh->name, ai_mesh->mName.data);
		TEMP_ALLOC_N(t, mesh->positions, vertex_count);
		TEMP_ALLOC_N(t, mesh->normals, vertex_count);

		const aiVector3D *pos_in = ai_mesh->mVertices;
		Vec3 *pos_out = mesh->positions;
		for (U32 vertexI = 0; vertexI < vertex_count; vertexI++) {
			pos_out->x = pos_in->x;
			pos_out->y = pos_in->y;
			pos_out->z = pos_in->z;
			pos_out++, pos_in++;
		}

		const aiVector3D *norm_in = ai_mesh->mNormals;
		Vec3 *norm_out = mesh->normals;
		for (U32 vertexI = 0; vertexI < vertex_count; vertexI++) {
			norm_out->x = norm_in->x;
			norm_out->y = norm_in->y;
			norm_out->z = norm_in->z;
			norm_out++, norm_in++;
		}

		U32 texcoord_stream_count = 0;

		for (U32 texstreamI = 0; texstreamI < AI_MAX_NUMBER_OF_TEXTURECOORDS; texstreamI++) {

			aiVector3D *texcoords = ai_mesh->mTextureCoords[texstreamI];
			if (!texcoords)
				continue;

			U32 components = ai_mesh->mNumUVComponents[texstreamI];

			TEMP_ALLOC_N(t, mesh->texcoords[texstreamI], vertex_count * components);
			float *stream = mesh->texcoords[texstreamI];

			if (components == 1) {
				for (U32 vertexI = 0; vertexI < vertex_count; vertexI++) {
					*stream++ = texcoords[vertexI].x;
				}
			} else if (components == 2) {
				for (U32 vertexI = 0; vertexI < vertex_count; vertexI++) {
					*stream++ = texcoords[vertexI].x;
					*stream++ = texcoords[vertexI].y;
				}
			} else if (components == 3) {
				for (U32 vertexI = 0; vertexI < vertex_count; vertexI++) {
					*stream++ = texcoords[vertexI].x;
					*stream++ = texcoords[vertexI].y;
					*stream++ = texcoords[vertexI].z;
				}
			} else {
				assert(0 && "Unsupported amount of components");
			}

			texcoord_stream_count++;
		}

		mesh->texcoord_stream_count = texcoord_stream_count;

		const U32 face_count = (U32)ai_mesh->mNumFaces;
		const U32 index_count = face_count * 3;
		mesh->index_count = index_count;

		TEMP_ALLOC_N(t, mesh->indices, index_count);

		U16 *out_index = mesh->indices;
		for (U32 faceI = 0; faceI < face_count; faceI++) {
			aiFace ai_face = ai_mesh->mFaces[faceI];
			assert(ai_face.mNumIndices == 3);

			out_index[0] = (U16)ai_face.mIndices[0];
			out_index[1] = (U16)ai_face.mIndices[1];
			out_index[2] = (U16)ai_face.mIndices[2];
			out_index += 3;
		}

		const U32 bone_count = (U32)ai_mesh->mNumBones;
		mesh->bone_count = bone_count;

		U32 bones_per_vertex = 0;

		// Refactor: Temporary buffer support to temporary allocator
		U32 *bones_per_vertices = (U32*)calloc(vertex_count, sizeof(U32));

		TEMP_ALLOC_N(t, mesh->bones, bone_count);
		for (U32 boneI = 0; boneI < bone_count; boneI++) {
			const aiBone *ai_bone = ai_mesh->mBones[boneI];
			Bone *bone = &mesh->bones[boneI];

			TEMP_COPY_STR(t, bone->name, ai_bone->mName.data);
			bone->inv_bind_pose_transform = translate_matrix(ai_bone->mOffsetMatrix);

			U32 weight_count = (U32)ai_bone->mNumWeights;
			for (U32 weightI = 0; weightI < weight_count; weightI++) {
				U32 count = ++bones_per_vertices[ai_bone->mWeights[weightI].mVertexId];
				if (bones_per_vertex < count)
					bones_per_vertex = count;
			}
		}

		free(bones_per_vertices);

		mesh->bones_per_vertex = bones_per_vertex;

		if (bones_per_vertex > 0) {
			U32 bone_weight_count = vertex_count * bones_per_vertex;
			TEMP_ALLOC_N(t, mesh->bone_indices, bone_weight_count);
			TEMP_ALLOC_N(t, mesh->bone_weights, bone_weight_count);

			for (U32 boneI = 0; boneI < bone_count; boneI++) {
				const aiBone *ai_bone = ai_mesh->mBones[boneI];

				const U32 vertex_count = ai_bone->mNumWeights;
				for (U32 vertexI = 0; vertexI < vertex_count; vertexI++) {
					aiVertexWeight ai_weight = ai_bone->mWeights[vertexI];

					U32 weight_index = 0;
					U32 base_vertex = (U32)ai_weight.mVertexId * bones_per_vertex;

					for (; weight_index < bones_per_vertex; weight_index++) {
						if (mesh->bone_weights[base_vertex + weight_index] == 0.0f) {
							break;
						}
					}
					assert(weight_index < bones_per_vertex);

					mesh->bone_indices[base_vertex + weight_index] = (U8)boneI;
					mesh->bone_weights[base_vertex + weight_index] = ai_weight.mWeight;
				}
			}
		}
	}

	TEMP_ALLOC_N(t, temp_data->nodes, 1024);
	Node *root_node = create_nodes_recursive(t, temp_data, scene->mRootNode);
	TEMP_POINTER_SET(t, temp_data->root_node, root_node);

	void *allocation = temp_allocator_finalize(t);
	if (!allocation)
		return 0;

	temp_data->allocation = allocation;

	return temp_data;
}

void free_model_file(Model_File_Data *data)
{
	free(data->allocation);
}

