uniform mat4 uViewProjection;
uniform mat4 uBones[NUM_BONES];
uniform mat4 uBonesIT[NUM_BONES];

attribute vec3 aVertex;
attribute vec3 aNormal;
attribute vec2 aTexCoord;

attribute vec4 aBoneIndex;
attribute vec4 aBoneWeight;

varying vec3 vNormal;

void main()
{
	vec4 vert = vec4(aVertex, 1.0);
	vec4 norm = vec4(aNormal, 0.0);

#if NUM_WEIGHTS == 0
	vec4 pos = vert;
	vec4 nrm = norm;
#elif NUM_WEIGHTS == 1
	vec4 pos = uBones[int(aBoneIndex.x)] * vert;
	vec4 nrm = uBonesIT[int(aBoneIndex.x)] * norm;
#elif NUM_WEIGHTS == 2
	vec4 pos = uBones[int(aBoneIndex.x)] * vert * aBoneWeight.x
	         + uBones[int(aBoneIndex.y)] * vert * aBoneWeight.y;
	
	vec4 nrm = uBonesIT[int(aBoneIndex.x)] * norm * aBoneWeight.x
	         + uBonesIT[int(aBoneIndex.y)] * norm * aBoneWeight.y;
#elif NUM_WEIGHTS == 3
	vec4 pos = uBones[int(aBoneIndex.x)] * vert * aBoneWeight.x
	         + uBones[int(aBoneIndex.y)] * vert * aBoneWeight.y
	         + uBones[int(aBoneIndex.z)] * vert * aBoneWeight.z;

	vec4 nrm = uBonesIT[int(aBoneIndex.x)] * norm * aBoneWeight.x
	         + uBonesIT[int(aBoneIndex.y)] * norm * aBoneWeight.y
	         + uBonesIT[int(aBoneIndex.z)] * norm * aBoneWeight.z;
#elif NUM_WEIGHTS == 4
	vec4 pos = uBones[int(aBoneIndex.x)] * vert * aBoneWeight.x
	         + uBones[int(aBoneIndex.y)] * vert * aBoneWeight.y
	         + uBones[int(aBoneIndex.z)] * vert * aBoneWeight.z
	         + uBones[int(aBoneIndex.w)] * vert * aBoneWeight.w;

	vec4 nrm = uBonesIT[int(aBoneIndex.x)] * norm * aBoneWeight.x
	         + uBonesIT[int(aBoneIndex.y)] * norm * aBoneWeight.y
	         + uBonesIT[int(aBoneIndex.z)] * norm * aBoneWeight.z
	         + uBonesIT[int(aBoneIndex.w)] * norm * aBoneWeight.w;
#else
#endif

	vNormal = nrm.xyz;
	gl_Position = uViewProjection * pos;
}

