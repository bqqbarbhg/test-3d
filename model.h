
// TODO: Remove these
typedef unsigned int U32;
typedef unsigned short U16;
typedef unsigned char U8;

#define MAX_TEXCOORD_STREAMS 4

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
	float *texcoords[MAX_TEXCOORD_STREAMS];
	U32 texcoord_components[MAX_TEXCOORD_STREAMS];
	U32 vertex_count;

	U32 texcoord_stream_count;

	U8 *bone_indices;
	float *bone_weights;
	U32 bones_per_vertex;

	U16 *indices;
	U32 index_count;

	Bone *bones;
	U32 bone_count;
};

struct Node
{
	const char *name;

	Node *parent;

	Node **children;
	U32 child_count;

	Mesh **meshes;
	U32 mesh_count;

	Mat44 transform;
};

struct Model_File_Data
{
	void *allocation;
	Mesh *meshes;
	U32 mesh_count;

	Node *nodes;
	U32 node_count;

	Node *root_node;
};

typedef void (*model_progress_callback)(float);

struct Model_File_Settings
{
	model_progress_callback progress_callback;
	Temp_Allocator *temp_allocator;
};

Model_File_Data *load_model_file(const char *file, const Model_File_Settings *settings);
void free_model_file(Model_File_Data *data);

