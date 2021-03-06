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

#define DEBUG_VALUE(val) ImGui::Value(#val, (val))
#include <imgui.h>

#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui_impl_glfw.cpp"
#include "temp_allocator.cpp"
#include "math.cpp"
#include "collision.cpp"
#include "debug_draw.cpp"
#include "editor_widget.cpp"
#include "model.cpp"
#include "streams.cpp"
#include "opengl.cpp"
#include "opengl_processing.cpp"
#include "editor_main.cpp"

