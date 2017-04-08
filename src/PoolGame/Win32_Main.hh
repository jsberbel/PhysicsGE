#pragma once

#include "Game.hh"

#include <Windows.h>

// opengl math library
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// opengl extension loading library
#include <GL/glew.h>
#include "GL/wglew.h"

// freetype library
#include <ft2build.h>
#include FT_FREETYPE_H 

// imgui library
#include "imgui/imgui.h"

#include "Profiler.hh"

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "Winmm.lib")
#ifdef _DEBUG
	#pragma comment (lib, "freetype271d.lib")
#else
	#pragma comment (lib, "freetype271.lib")
#endif

#ifdef NDEBUG
#define Assert(cnd, ...) (cnd)
#else
#define Assert(cnd, ...) \
		([&](decltype(cnd) &&c, const char* const f) { \
			if (!!c) return c; \
			char buffer[512]; \
			sprintf_s(buffer, "File: %s\nFunction: %-30.30s\nLine: %-5d\nContent: %s\nDescription: %s", __FILE__, f, __LINE__, #cnd, ##__VA_ARGS__, " "); \
			MessageBoxA(nullptr, buffer, nullptr, MB_ICONERROR); \
			exit(-1); \
		})(cnd, __FUNCTION__);
#endif

namespace Win32
{
	struct VertexTN
	{
		glm::vec3 p, n;
		glm::vec4 c;
		glm::vec2 t;
	};

	struct VAO
	{
		GLuint vao;
		GLuint ebo, vbo;
		int numIndices;
	};

	struct Renderer
	{
		static constexpr int BuffersSize { 0x10000 };
		static constexpr int DearImgui { 0x01 };

		enum {
			//VERTEX_ARRAY_COUNT = 0x02,
			VAO_COUNT = 0x02,
			UNIFORM_COUNT = 0x02,
			PROGRAM_COUNT = 0x02,
			TEXTURE_COUNT = static_cast<int>(Game::RenderData::TextureID::COUNT) + 0x01,
			COUNT
		};

		//GLuint vertexArrays[VERTEX_ARRAY_COUNT];
		VAO vaos[VAO_COUNT];
		GLuint uniforms[UNIFORM_COUNT];
		GLuint programs[PROGRAM_COUNT];
		GLuint textures[TEXTURE_COUNT];
	};

	struct InstanceData
	{
		glm::mat4 modelMatrix;
		glm::vec4 colorModifier;
	};

	//struct GLData
	//{
	//	// pack all render-related data into this struct
	//	GLuint program;
	//	VAO quadVAO;
	//	GLuint instanceDataBuffer;

	//	// map of all textures available
	//	GLuint textures[static_cast<int>(Game::RenderData::TextureID::COUNT)];
	//};

	void GenerateWindow(HINSTANCE hInstance, const wchar_t *name);
	GLuint GenerateProgram(const wchar_t *vertexShader, const wchar_t *fragmentShader);
}

