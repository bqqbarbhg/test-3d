
bool make_skinned_mesh(GL_Skinned_Mesh *gl_mesh, Mesh *mesh)
{
	// TODO: Sorting by bones etc. Should be done in processing?

	U32 vertex_size = skinned_vertex_size;
	U32 weight_count = mesh->bones_per_vertex;

	U32 vertex_count = mesh->vertex_count;
	gl_mesh->bone_count = mesh->bone_count;
	gl_mesh->weight_count = weight_count;
	gl_mesh->vertex_count = vertex_count;

	gl_mesh->vertex_buffer = 0;
	gl_mesh->index_buffer = 0;

	float *vertex_data = (float*)malloc(vertex_count * vertex_size * sizeof(float));

	for (U32 i = 0; i < vertex_count; i++) {
		float *f32 = &vertex_data[i * vertex_size];

		Vec3 *v = &mesh->positions[i];
		f32[0] = v->x;
		f32[1] = v->y;
		f32[2] = v->z;

		Vec3 *n = &mesh->normals[i];
		f32[3] = n->x;
		f32[4] = n->y;
		f32[5] = n->z;

		// FIXME: Support multiple texcoords (and don't expect them!)
#if 0
		float *t = &mesh->texcoords[0][i * mesh->texcoord_components[0]];
		f32[6] = t[0];
		f32[7] = t[1];
#endif

		unsigned char *bones = (unsigned char *)&f32[8];
		unsigned char *weights = (unsigned char *)&f32[9];
		if (weight_count == 1) {
			bones[0] = mesh->bone_indices[i * weight_count + 0];
			bones[1] = 0;
			bones[2] = 0;
			bones[3] = 0;
			weights[0] = 255;
			weights[1] = 0;
			weights[2] = 0;
			weights[3] = 0;
		} else if (weight_count == 2) {
			bones[0] = mesh->bone_indices[i * weight_count + 0];
			bones[1] = mesh->bone_indices[i * weight_count + 1];
			bones[2] = 0;
			bones[3] = 0;
			weights[0] = (U8)(mesh->bone_weights[i * weight_count + 0] * 255.0f + 0.5f);
			weights[1] = (U8)(mesh->bone_weights[i * weight_count + 1] * 255.0f + 0.5f);
			weights[2] = 0;
			weights[3] = 0;
		} else if (weight_count == 3) {
			bones[0] = mesh->bone_indices[i * weight_count + 0];
			bones[1] = mesh->bone_indices[i * weight_count + 1];
			bones[2] = mesh->bone_indices[i * weight_count + 2];
			bones[3] = 0;
			weights[0] = (U8)(mesh->bone_weights[i * weight_count + 0] * 255.0f + 0.5f);
			weights[1] = (U8)(mesh->bone_weights[i * weight_count + 1] * 255.0f + 0.5f);
			weights[2] = (U8)(mesh->bone_weights[i * weight_count + 2] * 255.0f + 0.5f);
			weights[3] = 0;
		} else if (weight_count == 4) {
			bones[0] = mesh->bone_indices[i * weight_count + 0];
			bones[1] = mesh->bone_indices[i * weight_count + 1];
			bones[2] = mesh->bone_indices[i * weight_count + 2];
			bones[3] = mesh->bone_indices[i * weight_count + 3];
			weights[0] = (U8)(mesh->bone_weights[i * weight_count + 0] * 255.0f + 0.5f);
			weights[1] = (U8)(mesh->bone_weights[i * weight_count + 1] * 255.0f + 0.5f);
			weights[2] = (U8)(mesh->bone_weights[i * weight_count + 2] * 255.0f + 0.5f);
			weights[3] = (U8)(mesh->bone_weights[i * weight_count + 3] * 255.0f + 0.5f);
		} else {
			assert(0 && "Unexpected weight count");
		}
	}
	gl_mesh->vertices = vertex_data;

	U32 index_count = mesh->index_count;
	gl_mesh->index_count = index_count;

	U32 *wide_indices = mesh->indices;

	if (vertex_count < 1 << 8) {
		gl_mesh->index_type = GL_UNSIGNED_BYTE;

		GLubyte *indices = (GLubyte*)malloc(index_count * sizeof(GLubyte));
		for (U32 i = 0; i < index_count; i++) {
			indices[i] = (GLubyte)wide_indices[i];
		}
		gl_mesh->indices = indices;
	} else if (mesh->vertex_count < 1 << 16) {
		gl_mesh->index_type = GL_UNSIGNED_SHORT;

		GLushort *indices = (GLushort*)malloc(index_count * sizeof(GLushort));
		for (U32 i = 0; i < index_count; i++) {
			indices[i] = (GLushort)wide_indices[i];
		}
		gl_mesh->indices = indices;
	} else {
		gl_mesh->index_type = GL_UNSIGNED_INT;
		
		GLubyte *indices = (GLubyte*)malloc(index_count * sizeof(GLubyte));
		for (U32 i = 0; i < index_count; i++) {
			indices[i] = (GLubyte)wide_indices[i];
		}
		gl_mesh->indices = indices;
	}

	return true;
}

