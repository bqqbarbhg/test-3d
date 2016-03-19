#include "prelude.h"
#include "intrinsics.h"

#if defined(_WIN32)
	#define GLEW_STATIC
	#include <GL/glew.h>
	#define NOMINMAX
	#include <Windows.h>
	#include <windef.h>
	#include <GL/gl.h>
#elif defined(__APPLE__)
	#include <OpenGL/gl.h>
#else
	#error "Unknown platform"
#endif

#include "math.cpp"
#include "collision.cpp"
#include "streams.cpp"
#include "opengl.cpp"
#include "viewer_main.cpp"

