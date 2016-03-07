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
	// TODO!
	return ret;
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

		TEMP_COPY_STR(t, mesh->name, ai_mesh->mName.data);
		TEMP_ALLOC_N(t, mesh->positions, vertex_count);
		TEMP_ALLOC_N(t, mesh->normals, vertex_count);
		TEMP_ALLOC_N(t, mesh->texcoords, vertex_count);
		mesh->vertex_count = vertex_count;

		const U32 bone_count = ai_mesh->mNumBones;
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

