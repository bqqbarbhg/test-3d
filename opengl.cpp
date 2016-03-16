
struct Shader_Define
{
	const char *name;
	int value;
};

void shader_source_defines(GLint shader, const char *source, int size, const Shader_Define *defines, size_t define_count)
{
	char prefix[2048], *prefix_ptr = prefix;
	for (size_t i = 0; i < define_count; i++) {
		const Shader_Define *d = &defines[i];
		prefix_ptr += sprintf(prefix_ptr, "#define %s %d\n", d->name, d->value);
	}

	const GLchar* sources[2] = { (const GLchar*)prefix, (const GLchar*)source };
	GLint lengths[2] = { (GLint)(prefix_ptr - prefix), (GLint)size };
	glShaderSource(shader, 2, sources, lengths);
}

void debug_compile_shader(GLint shader)
{
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char info_log[2048];
		glGetShaderInfoLog(shader, Count(info_log), 0, info_log);
		fprintf(stderr, "%s\n", info_log);
	}
}
void debug_link_program(GLint program)
{
	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		char info_log[2048];
		glGetProgramInfoLog(program, Count(info_log), 0, info_log);
		fprintf(stderr, "%s\n", info_log);
	}
}

char* debug_read_file(const char *path)
{
	FILE *file = fopen(path, "rb");
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *buffer = (char*)malloc(size + 1);
	fread(buffer, size, 1, file);
	buffer[size] = '\0';

	fclose(file);
	return buffer;
}

int max_bones = 40;

const char *GLSL_NUM_BONES = "NUM_BONES";
const char *GLSL_NUM_WEIGHTS = "NUM_WEIGHTS";

GLint skinned_vertex_shaders[4];
GLint skinned_vertex_programs[4];
GLint skinned_frag_shader;

void generate_shaders()
{
	char *skinned_vert_src = debug_read_file("data/shader/skinned_mesh.vert");
	char *skinned_frag_src = debug_read_file("data/shader/skinned_mesh.frag");

	skinned_frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(skinned_frag_shader, 1, &skinned_frag_src, 0);
	debug_compile_shader(skinned_frag_shader);

	for (int weightI = 0; weightI < 4; weightI++) {
		Shader_Define defines[] = {
			{ GLSL_NUM_BONES, max_bones },
			{ GLSL_NUM_WEIGHTS, weightI + 1 },
		};

		GLint shader = glCreateShader(GL_VERTEX_SHADER);
		shader_source_defines(shader, skinned_vert_src, -1, defines, Count(defines));
		debug_compile_shader(shader);

		GLint program = glCreateProgram();
		glAttachShader(program, shader);
		glAttachShader(program, skinned_frag_shader);
		debug_link_program(program);

		skinned_vertex_shaders[weightI] = shader;
		skinned_vertex_programs[weightI] = program;
	}

	free(skinned_vert_src);
	free(skinned_frag_src);
}

struct GL_Skinned_Mesh
{
	GLuint vertex_buffer, index_buffer;
	U32 index_bytes;
	U32 bone_count;
};

bool make_skinned_mesh(GL_Skinned_Mesh *gl_mesh, Mesh *mesh)
{
	// TODO: Sorting by bones etc. Should be done in processing?

	glGenBuffers(1, &gl_mesh->vertex_buffer);
	glGenBuffers(1, &gl_mesh->index_buffer);

	U32 weight_count = mesh->bones_per_vertex;
	U32 vertex_size = 3 + 3 + 2 + 1 + 1;

	U32 vertex_count = mesh->vertex_count;
	gl_mesh->bone_count = mesh->bone_count;

	float *vertex_data = (float*)malloc(vertex_count * vertex_size * sizeof(float));

	for (U32 i = 0; i < vertex_count; i++) {
		float *f32 = &vertex_data[i * vertex_size];

		Vec3 *v = &mesh->positions[i];
		f32[0] = v->x;
		f32[1] = v->y;
		f32[2] = v->z;

		Vec3 *n = &mesh->normals[i];
		f32[3] = v->x;
		f32[4] = v->y;
		f32[5] = v->z;

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
	glBindBuffer(GL_ARRAY_BUFFER, gl_mesh->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * vertex_size * sizeof(float), vertex_data, GL_STATIC_DRAW);

	free(vertex_data);

	U32 index_count = mesh->index_count;
	U32 *wide_indices = mesh->indices;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_mesh->index_buffer);
	if (vertex_count < 1 << 8) {
		gl_mesh->index_bytes = 1;

		GLubyte *indices = (GLubyte*)malloc(index_count * sizeof(GLubyte));
		for (U32 i = 0; i < index_count; i++) {
			indices[i] = (GLubyte)wide_indices[i];
		}
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(GLubyte), indices, GL_STATIC_DRAW);
		free(indices);
	} else if (mesh->vertex_count < 1 << 16) {
		gl_mesh->index_bytes = 2;

		GLushort *indices = (GLushort*)malloc(index_count * sizeof(GLushort));
		for (U32 i = 0; i < index_count; i++) {
			indices[i] = (GLushort)wide_indices[i];
		}
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(GLushort), indices, GL_STATIC_DRAW);
		free(indices);
	} else if (mesh->vertex_count < 1 << 16) {
	} else {
		gl_mesh->index_bytes = 4;
		
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(GLuint), wide_indices, GL_STATIC_DRAW);
	}

	return true;
}

