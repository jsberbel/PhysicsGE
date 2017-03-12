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

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "Winmm.lib")
#pragma comment (lib, "freetype271d.lib")

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

struct InstanceData
{
	glm::mat4 modelMatrix;
	glm::vec4 colorModifier;
};

struct GLData
{
	// pack all render-related data into this struct
	GLuint program;
	VAO quadVAO;
	GLuint instanceDataBuffer;

	// map of all textures available
	GLuint textures[static_cast<int>(Game::RenderData::TextureID::COUNT)];
};

struct Character {
	GLuint     TextureID;  // ID handle of the glyph texture
	glm::ivec2 Size;       // Size of glyph
	glm::ivec2 Bearing;    // Offset from baseline to left/top of glyph
	GLuint     Advance;    // Offset to advance to next glyph
};