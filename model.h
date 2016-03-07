
struct Mat44
{
};

struct Vec2
{
	float x, y;
};

struct Vec3
{
	float x, y, z;
};

// TODO: Remove these
typedef unsigned int U32;
typedef unsigned char U8;

struct Bone
{
	const char *name;
	Mat44 inv_bind_pose_transform;
};

struct Mesh
{
	const char *name;

	Vec3 *positions;
	Vec3 *normals;
	Vec2 *texcoords;
	U32 vertex_count;

	U8 *bone_indices;
	float *bone_weights;
	U32 bones_per_vertex;

	Bone *bones;
	U32 bone_count;
};

struct Model_File_Data
{
	void *allocation;
	Mesh *meshes;
	U32 mesh_count;
};

typedef void (*model_progress_callback)(float);

struct Model_File_Settings
{
	model_progress_callback progress_callback;
	Temp_Allocator *temp_allocator;
};

Model_File_Data *load_model_file(const char *file, const Model_File_Settings *settings);
void free_model_file(Model_File_Data *data);

