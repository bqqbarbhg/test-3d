#if defined(_WIN32)
	#include <Windows.h>
	#include <windef.h>
	#include <GL/gl.h>
#elif defined(__APPLE__)
	#include <OpenGL/gl.h>
#else
	#error "Unknown platform"
#endif

#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui_impl_glfw.cpp"
#include "temp_allocator.cpp"
#include "math.cpp"
#include "model.cpp"
#include "main.cpp"

