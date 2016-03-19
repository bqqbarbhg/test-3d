
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

bool debug_compile_shader(GLint shader)
{
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char info_log[2048];
		glGetShaderInfoLog(shader, Count(info_log), 0, info_log);
		fprintf(stderr, "%s\n", info_log);
		return false;
	}
	return true;
}
bool debug_link_program(GLint program)
{
	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		char info_log[2048];
		glGetProgramInfoLog(program, Count(info_log), 0, info_log);
		fprintf(stderr, "%s\n", info_log);
		return false;
	}
	return true;
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

#define GL_MAX_BONES 20

const char *GLSL_NUM_BONES = "NUM_BONES";
const char *GLSL_NUM_WEIGHTS = "NUM_WEIGHTS";

GLint skinned_frag_shader;
struct Skinned_Shader
{
	GLuint vert_shader;
	GLuint program;

	GLint uViewProjection;
	GLint uBones;
	GLint uBonesIT;

	GLint aVertex;
	GLint aNormal;
	GLint aTexCoord;
	GLint aBoneIndex;
	GLint aBoneWeight;
};

Skinned_Shader skinned_shaders[5];

bool generate_shaders()
{
	char *skinned_vert_src = debug_read_file("data/shader/skinned_mesh.vert");
	char *skinned_frag_src = debug_read_file("data/shader/skinned_mesh.frag");

	skinned_frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(skinned_frag_shader, 1, &skinned_frag_src, 0);
	if (!debug_compile_shader(skinned_frag_shader))
		return false;

	for (int weightI = 0; weightI < 5; weightI++) {
		Shader_Define defines[] = {
			{ GLSL_NUM_BONES, GL_MAX_BONES },
			{ GLSL_NUM_WEIGHTS, weightI },
		};

		Skinned_Shader *s = &skinned_shaders[weightI];

		GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
		s->vert_shader = vert_shader;
		shader_source_defines(vert_shader, skinned_vert_src, -1, defines, Count(defines));
		if (!debug_compile_shader(vert_shader))
			return false;

		GLint program = glCreateProgram();
		s->program = program;

		glAttachShader(program, vert_shader);
		glAttachShader(program, skinned_frag_shader);

		if (!debug_link_program(program))
			return false;

		s->uViewProjection = glGetUniformLocation(program, "uViewProjection");
		s->uBones = glGetUniformLocation(program, "uBones");
		s->uBonesIT = glGetUniformLocation(program, "uBonesIT");

		s->aVertex = glGetAttribLocation(program, "aVertex");
		s->aNormal = glGetAttribLocation(program, "aNormal");
		s->aTexCoord = glGetAttribLocation(program, "aTexCoord");
		s->aBoneIndex = glGetAttribLocation(program, "aBoneIndex");
		s->aBoneWeight = glGetAttribLocation(program, "aBoneWeight");
	}

	free(skinned_vert_src);
	free(skinned_frag_src);
	return true;
}

const U32 skinned_vertex_size = 3 + 3 + 2 + 1 + 1;

struct GL_Skinned_Mesh
{
	GLuint vertex_buffer, index_buffer;
	void *vertices, *indices;

	U32 vertex_count;
	U32 index_count;

	GLint index_type;
	U32 bone_count;
	U32 weight_count;
};

int gl_type_size(GLint type)
{
	switch (type) {
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
			return 1;
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
			return 2;
		case GL_INT:
		case GL_UNSIGNED_INT:
			return 4;
		default:
			assert(0 && "Unexpected type");
			return 0;
	}
}

void do_load_skinned_mesh_to_gl(GL_Skinned_Mesh *mesh)
{
	assert(mesh->vertices != 0);
	assert(mesh->indices != 0);
	assert(mesh->vertex_buffer == 0);
	assert(mesh->index_buffer == 0);

	glGenBuffers(1, &mesh->vertex_buffer);
	glGenBuffers(1, &mesh->index_buffer);

	glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer);

	GLsizei vertex_size = sizeof(float) * skinned_vertex_size * mesh->vertex_count;
	glBufferData(GL_ARRAY_BUFFER, vertex_size, mesh->vertices, GL_STATIC_DRAW);
	GLsizei index_size = gl_type_size(mesh->index_type) * mesh->index_count;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size, mesh->indices, GL_STATIC_DRAW);
}

void load_skinned_mesh_to_gl(GL_Skinned_Mesh *mesh)
{
	do_load_skinned_mesh_to_gl(mesh);

	free(mesh->vertices);
	free(mesh->indices);

	mesh->vertices = 0;
	mesh->indices = 0;
}

void write_skinned_mesh(Out_Stream *s, GL_Skinned_Mesh *mesh)
{
	stream_write(s, &mesh->vertex_count, sizeof(U32));
	stream_write(s, &mesh->index_count, sizeof(U32));
	stream_write(s, &mesh->index_type, sizeof(GLint));
	stream_write(s, &mesh->bone_count, sizeof(U32));
	stream_write(s, &mesh->weight_count, sizeof(U32));
	stream_write(s, mesh->vertices, skinned_vertex_size * sizeof(float), mesh->vertex_count);
	stream_write(s, mesh->indices, gl_type_size(mesh->index_type), mesh->index_count);
}

void read_skinned_mesh_to_gl(In_Stream *s, GL_Skinned_Mesh *mesh)
{
	stream_read(s, &mesh->vertex_count, sizeof(U32));
	stream_read(s, &mesh->index_count, sizeof(U32));
	stream_read(s, &mesh->index_type, sizeof(GLint));
	stream_read(s, &mesh->bone_count, sizeof(U32));
	stream_read(s, &mesh->weight_count, sizeof(U32));

	mesh->vertices = stream_skip(s, skinned_vertex_size * sizeof(float), mesh->vertex_count);
	mesh->indices = stream_skip(s, gl_type_size(mesh->index_type), mesh->index_count);

	do_load_skinned_mesh_to_gl(mesh);

	mesh->vertices = 0;
	mesh->indices = 0;
}

void draw_skinned_mesh(GL_Skinned_Mesh *mesh, const Mat44& viewProjection, const Mat44 *transforms)
{
	Skinned_Shader *s = &skinned_shaders[mesh->weight_count];

	glUseProgram(s->program);
	if (s->uViewProjection >= 0)
		glUniformMatrix4fv(s->uViewProjection, 1, GL_FALSE, (const GLfloat*)&viewProjection);
	if (s->uBones >= 0)
		glUniformMatrix4fv(s->uBones, mesh->bone_count, GL_FALSE, (const GLfloat*)transforms);
	if (s->uBonesIT >= 0) {
		Mat44 transIt[GL_MAX_BONES];
		for (U32 i = 0; i < mesh->bone_count; i++)
			transIt[i] = transpose(inverse(transforms[i]));
		glUniformMatrix4fv(s->uBonesIT, mesh->bone_count, GL_FALSE, (const GLfloat*)transIt);
	}

	glBindBuffer(GL_ARRAY_BUFFER, mesh->vertex_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->index_buffer);

	GLuint sz = skinned_vertex_size * sizeof(float);

	if (s->aVertex >= 0) {
		glEnableVertexAttribArray(s->aVertex);
		glVertexAttribPointer(s->aVertex, 3, GL_FLOAT, GL_FALSE, sz, (const GLvoid*)(0 * sizeof(float)));
	}
	if (s->aNormal >= 0) {
		glEnableVertexAttribArray(s->aNormal);
		glVertexAttribPointer(s->aNormal, 3, GL_FLOAT, GL_FALSE, sz, (const GLvoid*)(3 * sizeof(float)));
	}
	if (s->aTexCoord >= 0) {
		glEnableVertexAttribArray(s->aTexCoord);
		glVertexAttribPointer(s->aTexCoord, 2, GL_FLOAT, GL_FALSE, sz, (const GLvoid*)(6 * sizeof(float)));
	}
	if (s->aBoneIndex >= 0) {
		glEnableVertexAttribArray(s->aBoneIndex);
		glVertexAttribPointer(s->aBoneIndex, 4, GL_UNSIGNED_BYTE, GL_FALSE, sz, (const GLvoid*)(8 * sizeof(float)));
	}
	if (s->aBoneWeight >= 0) {
		glEnableVertexAttribArray(s->aBoneWeight);
		glVertexAttribPointer(s->aBoneWeight, 4, GL_UNSIGNED_BYTE, GL_TRUE, sz, (const GLvoid*)(9 * sizeof(float)));
	}

	glDrawElements(GL_TRIANGLES, mesh->index_count, mesh->index_type, (const GLvoid*)0);
}

