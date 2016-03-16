
uniform mat4 uViewProjection;
uniform mat4 uBones[NUM_BONES];

attribute vec4 aVertex;
attribute vec2 aTexCoord;

attribute ivec4 aBoneIndex;
attribute vec4 aBoneWeight;

void main()
{
#if NUM_WEIGHTS == 1
	vec4 pos = aVertex * uBones[aBoneIndex.x];
#elif NUM_WEIGHTS == 2
	vec4 pos = aVertex * uBones[aBoneIndex.x] * aBoneWeight.x
	         + aVertex * uBones[aBoneIndex.y] * aBoneWeight.y;
#elif NUM_WEIGHTS == 3
	vec4 pos = aVertex * uBones[aBoneIndex.x] * aBoneWeight.x
	         + aVertex * uBones[aBoneIndex.y] * aBoneWeight.y
	         + aVertex * uBones[aBoneIndex.z] * aBoneWeight.z;
#elif NUM_WEIGHTS == 4
	vec4 pos = aVertex * uBones[aBoneIndex.x] * aBoneWeight.x
	         + aVertex * uBones[aBoneIndex.y] * aBoneWeight.y
	         + aVertex * uBones[aBoneIndex.z] * aBoneWeight.z
	         + aVertex * uBones[aBoneIndex.w] * aBoneWeight.w;
#else
#endif

	gl_Position = uViewProjection * pos;
}

