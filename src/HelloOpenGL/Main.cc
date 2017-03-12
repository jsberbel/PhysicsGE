#include <Windows.h>

#include "GL/glew.h"
#include "GL/wglew.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

#include "Game.hh"

#include <cassert>
#include <cstdio>

#define internal		static
#define local_persist	static
#define global_var		static

#ifdef NDEBUG
#define ASSERT(cnd, ...) (cnd)
#else
#define ASSERT(cnd, ...) \
		([&](decltype(cnd) &&c, const char* const f) { \
			if (!!c) return c; \
			char buffer[512]; \
			sprintf_s(buffer, "File: %s\nFunction: %-30.30s\nLine: %-5d\nContent: %s\nDescription: %s", __FILE__, f, __LINE__, #cnd, ##__VA_ARGS__, " "); \
			MessageBoxA(nullptr, buffer, nullptr, MB_ICONERROR); \
			exit(-1); \
		})(cnd, __FUNCTION__);
#endif

#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "Winmm.lib")

// Global Static Variables
global_var HGLRC s_OpenGLRenderingContext{ nullptr };
global_var HDC s_WindowHandleToDeviceContext;

global_var bool s_windowActive{ true };
global_var GLsizei s_screenWidth{ 640 }, s_screenHeight{ 480 };

inline void GLAPIENTRY openGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, 
										   const GLchar* message, const void* userParam)
{

	OutputDebugStringA("openGL Debug Callback : [");
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		OutputDebugStringA("ERROR");
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		OutputDebugStringA("DEPRECATED_BEHAVIOR");
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		OutputDebugStringA("UNDEFINED_BEHAVIOR");
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		OutputDebugStringA("PORTABILITY");
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		OutputDebugStringA("PERFORMANCE");
		break;
	case GL_DEBUG_TYPE_OTHER:
		OutputDebugStringA("OTHER");
		break;
	default:
		OutputDebugStringA("????");
		break;
	}
	OutputDebugStringA(":");
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_LOW:
		OutputDebugStringA("LOW");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		OutputDebugStringA("MEDIUM");
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		OutputDebugStringA("HIGH");
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		OutputDebugStringA("NOTIFICATION");
		break;
	default:
		OutputDebugStringA("????");
		break;
	}
	OutputDebugStringA(":");
	char buffer[512];
	sprintf_s(buffer, "%d", id);
	OutputDebugStringA(buffer);
	OutputDebugStringA("] ");
	OutputDebugStringA(message);
	OutputDebugStringA("\n");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_CREATE: {
		s_WindowHandleToDeviceContext = GetDC(hWnd);
		
		PIXELFORMATDESCRIPTOR pfd {
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
			PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
			32,                       //Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                       //Number of bits for the depthbuffer
			8,                        //Number of bits for the stencilbuffer
			0,                        //Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		int letWindowsChooseThisPixelFormat{ ChoosePixelFormat(s_WindowHandleToDeviceContext, &pfd) };
		SetPixelFormat(s_WindowHandleToDeviceContext, letWindowsChooseThisPixelFormat, &pfd);

		HGLRC tmpContext{ wglCreateContext(s_WindowHandleToDeviceContext) };
		wglMakeCurrent(s_WindowHandleToDeviceContext, tmpContext);

		// init glew
		GLenum err{ glewInit() };
		if (err != GLEW_OK)
			assert(false); // TODO

		int attribs[] {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 1,
			WGL_CONTEXT_FLAGS_ARB,
			/*WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB*/0
			#if _DEBUG
					| WGL_CONTEXT_DEBUG_BIT_ARB
			#endif
			, 0
		};

		s_OpenGLRenderingContext = wglCreateContextAttribsARB(s_WindowHandleToDeviceContext, nullptr, attribs);

		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(tmpContext);

		wglMakeCurrent(s_WindowHandleToDeviceContext, s_OpenGLRenderingContext);

		if (GLEW_ARB_debug_output)
		{
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallbackARB(openGLDebugCallback, nullptr);
			GLuint unusedIds{ 0 };
			glDebugMessageControl(GL_DONT_CARE,
				GL_DONT_CARE,
				GL_DONT_CARE,
				0,
				&unusedIds,
				true);
		}

		//wglSwapIntervalEXT(1);

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		#ifndef _DEBUG
			ToggleFullscreen(hWnd);
		#endif
	}
	break;
	case WM_DESTROY:
		wglDeleteContext(s_OpenGLRenderingContext);
		s_OpenGLRenderingContext = nullptr;
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_ESCAPE:
			PostQuitMessage(0);
			return 0;
		} break;
	}
	case WM_SIZE: {
		//s_screenWidth = LOWORD(lParam);  // Macro to get the low-order word.
		//s_screenHeight = HIWORD(lParam); // Macro to get the high-order word.
		glViewport(0, 0, s_screenWidth = LOWORD(lParam), s_screenHeight = HIWORD(lParam));
		break;
	}
	case WM_ACTIVATE: {
		s_windowActive = wParam != WA_INACTIVE;
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool CompileShader(GLuint &shaderId_, const uint8_t* shaderCode, GLenum shaderType)
{
	shaderId_ = glCreateShader(shaderType);
	glShaderSource(shaderId_, 1, reinterpret_cast<const GLchar *const*>(&shaderCode), nullptr);
	glCompileShader(shaderId_);

	GLint success = 0;
	glGetShaderiv(shaderId_, GL_COMPILE_STATUS, &success);

	{
		GLint maxLength = 0;
		glGetShaderiv(shaderId_, GL_INFO_LOG_LENGTH, &maxLength);

		if (maxLength > 0)
		{
			//The maxLength includes the NULL character
			GLchar *infoLog = new GLchar[maxLength];
			glGetShaderInfoLog(shaderId_, maxLength, &maxLength, &infoLog[0]);

			OutputDebugStringA(infoLog);
			OutputDebugStringA("\n");

			delete[] infoLog;
		}
	}
	return (success != GL_FALSE);
}

bool LinkShaders(GLuint &programId_, GLuint &vs, GLuint &ps)
{
	programId_ = glCreateProgram();

	glAttachShader(programId_, vs);

	glAttachShader(programId_, ps);

	glLinkProgram(programId_);

	GLint success{ 0 };
	glGetProgramiv(programId_, GL_LINK_STATUS, &success);
	{
		GLint maxLength{ 0 };
		glGetProgramiv(programId_, GL_INFO_LOG_LENGTH, &maxLength);

		if (maxLength > 0)
		{
			//The maxLength includes the NULL character
			GLchar *infoLog{ new GLchar[maxLength] };
			glGetProgramInfoLog(programId_, maxLength, &maxLength, &infoLog[0]);

			OutputDebugStringA(infoLog);
			OutputDebugStringA("\n");

			delete[] infoLog;
		}
	}
	return (success != GL_FALSE);
}

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

template <GLuint type, GLenum usage = GL_STATIC_DRAW>
inline void CreateBuffer(GLuint &buffer_, const void *data, GLsizei size)
{
	glGenBuffers(1, &buffer_); // creates a buffer
	glBindBuffer(type, buffer_);
	glBufferData(  // sets the buffer content
				 type,	// type of buffer
				 size,				// size of the buffer content
				 data,						// content of the buffer
				 GL_STATIC_DRAW);			// usage of the buffer. DYNAMIC -> will change frequently. DRAW -> from CPU to GPU
	glBindBuffer(type, 0); // set no buffer as the current buffer
}

inline void CreateVertexArrayObject(VAO &vao_, const VertexTN * vertexs, int numVertexs, const uint16_t * indices, int numIndices)
{
	vao_.numIndices = numIndices;

	GLuint elementArrayBufferObject, vertexBufferObject;

	glGenVertexArrays(1, &vao_.vao);
	glGenBuffers(1, &elementArrayBufferObject);
	glGenBuffers(1, &vertexBufferObject);

	glBindVertexArray(vao_.vao);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBufferObject); // create an element array buffer (index buffer)
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * numIndices, indices, GL_STATIC_DRAW); // fill buffer data
																								   // size of data	ptr to data  usage

	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject); // create an array buffer (vertex buffer)
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexTN) * numVertexs, vertexs, GL_STATIC_DRAW); // fill buffer data

	glVertexAttribPointer(0, // Vertex Attrib Index
		3, GL_FLOAT, // 3 floats
		GL_FALSE, // no normalization
		sizeof(VertexTN), // offset from a vertex to the next
		reinterpret_cast<GLvoid*>(offsetof(VertexTN, p)) // offset from the start of the buffer to the first vertex
	); // positions
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexTN), reinterpret_cast<GLvoid*>(offsetof(VertexTN, c))); // colors
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTN), reinterpret_cast<GLvoid*>(offsetof(VertexTN, t))); // textures
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTN), reinterpret_cast<GLvoid*>(offsetof(VertexTN, n))); // normals

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	// reset default opengl state
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

inline void Render(const VAO &vao)
{
	glBindVertexArray(vao.vao);
	glDrawElements(GL_TRIANGLES, vao.numIndices, GL_UNSIGNED_SHORT, nullptr);
}

#include "IO.h"

inline void CreateShaders()
{
	auto fileVS{ ReadFullFile(L"shaders/Simple.vs") };
	auto filePS{ ReadFullFile(L"shaders/Simple.fs") };
	GLuint vs, ps, program;

	ASSERT(CompileShader(vs, fileVS.data, GL_VERTEX_SHADER) &&
		   CompileShader(ps, filePS.data, GL_FRAGMENT_SHADER) &&
		   LinkShaders(program, vs, ps));

	if (vs > 0)
		glDeleteShader(vs);
	if (ps > 0)
		glDeleteShader(ps);

	glUseProgram(program); // Installs a program object as part of current rendering state
}

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

inline void LoadTexture(GLuint &texture_, const char path[])
{
	int x, y, comp;
	unsigned char *data{ stbi_load(path, &x, &y, &comp, 4) };

	glGenTextures(1, &texture_);
	glBindTexture(GL_TEXTURE_2D, texture_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	stbi_image_free(data);
}

//int _tmain(int argc, _TCHAR* argv[])
int __stdcall WinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd)
{
	// LOAD WINDOW STUFF
	{
		WNDCLASS wc{ 0 }; // Contains the window class attributes
		wc.lpfnWndProc = WndProc; // A pointer to the window procedure (fn that processes msgs)
		wc.hInstance = hInstance; // A handle to the instance that contains the window procedure
		wc.hbrBackground = HBRUSH(COLOR_BACKGROUND); // A handle to the class background brush
		wc.lpszClassName = L"Hello World"; // Specifies the window class name
		wc.style = CS_OWNDC; // Class styles: CS_OWNDC -> Allocates a unique device context for each window in the class

		ASSERT (RegisterClass(&wc)); // Registers a window class for subsequent use in calls
		HWND hWnd{ CreateWindowW(wc.lpszClassName, L"Hello World", // Class name initialized before // Window name
								 WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Style of the window created
								 CW_USEDEFAULT, CW_USEDEFAULT, // Window position in device units
								 s_screenWidth, s_screenHeight, // Window size in device units
								 nullptr, nullptr, // Parent handle // Menu handle
								 hInstance, nullptr) }; // Handle to instance of the module // Pointer to winstuff
	}

	// INIT SHADERS
	CreateShaders();

	// INIT OBJECTS
	VAO quadVAO;
	{
		VertexTN vtxs[] {
			{ glm::vec3{ -1, -1, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec4{ 0, 1, 1, 1 }, glm::vec2{ 0,0 } },
			{ glm::vec3{ +1, -1, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec4{ 1, 0, 1, 1 }, glm::vec2{ 1,0 } },
			{ glm::vec3{ -1, +1, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec4{ 1, 0, 0, 1 }, glm::vec2{ 0,1 } },
			{ glm::vec3{ +1, +1, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec4{ 1, 1, 0, 1 }, glm::vec2{ 1,1 } }
		};
		uint16_t idxs[] {
			0, 1, 2,
			2, 1, 3
		};

		//CreateVertexBufferObject(quadVAO, vtxs);
		//CreateElementArrayBufferObject(quadVAO, idxs);
		CreateBuffer<GL_ARRAY_BUFFER>(quadVAO.vao, vtxs, sizeof vtxs);
		CreateBuffer<GL_ELEMENT_ARRAY_BUFFER>(quadVAO.ebo, idxs, sizeof idxs);
		CreateVertexArrayObject(quadVAO, vtxs, sizeof vtxs, idxs, sizeof idxs);
	}

	GLuint instanceDataBuffer;
	CreateBuffer<GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW>(instanceDataBuffer, nullptr, sizeof InstanceData);
	/*{
		glGenBuffers(1, &instanceDataBuffer); // creates a buffer. The buffer id is written to the variable "instanceDataBuffer"
		glBindBuffer(GL_UNIFORM_BUFFER, instanceDataBuffer); // sets "instanceDataBuffer" as the current buffer
		glBufferData(  // sets the buffer content
					 GL_UNIFORM_BUFFER,		// type of buffer
					 sizeof(InstanceData),	// size of the buffer content
					 nullptr,				// content of the buffer
					 GL_DYNAMIC_DRAW);		// usage of the buffer. DYNAMIC -> will change frequently. DRAW -> from CPU to GPU
											// we initialize the buffer without content so we can later call "glBufferSubData". 
											// Here we are only reserving the size.
		glBindBuffer(GL_UNIFORM_BUFFER, 0); // set no buffer as the current buffer
	}*/

	GLuint lenaTexture;
	LoadTexture(lenaTexture, "lena.png");

	// INPUT
	bool keyboard[256] {};
	glm::vec2 position {};
	float horizontal{ 0 }, vertical{ 0 };

	LARGE_INTEGER l_LastFrameTime;
	int64_t l_PerfCountFrequency;
	{
		LARGE_INTEGER PerfCountFrequencyResult;
		QueryPerformanceFrequency(&PerfCountFrequencyResult);
		l_PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	}
	int64_t l_TicksPerFrame = l_PerfCountFrequency / 120;
	double dt = 0.016f;
	
	bool quit{ false };
	do {
		// init time
		QueryPerformanceCounter(&l_LastFrameTime);

		// PROCESS MSGS
		{
			MSG msg {};

			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				bool processed{ false };
				if (msg.message == WM_QUIT)
				{
					quit = true;
					processed = true;
				}
				else if (msg.message == WM_KEYDOWN)
				{
					keyboard[msg.wParam & 255] = true;
					processed = true;
					if (msg.wParam == VK_ESCAPE)
					{
						PostQuitMessage(0);
					}
				}
				else if (msg.message == WM_KEYUP)
				{
					keyboard[msg.wParam & 255] = false;
					processed = true;
				}

				if (!processed)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}

			if (keyboard['A'])
				horizontal -= 0.1f;
			
			if (keyboard['D'])
				horizontal += 0.1f;

			if (keyboard['W'])
				vertical += 0.1f;
			
			if (keyboard['S'])
				vertical -= 0.1f;
			
			/*position.x += horizontal * 0.016f;
			position.y += vertical * 0.016f;*/
			position.x += horizontal * dt;
			position.y += vertical * dt;
		}

		{
			//InstanceData instanceData{ glm::mat4(), glm::vec4(1,0,1,1) };
			InstanceData instanceData{ glm::translate(glm::mat4(), glm::vec3(position, 0)), glm::vec4(1,1,1,1) };

			glBindBuffer(GL_UNIFORM_BUFFER, instanceDataBuffer);
			glBufferSubData(GL_UNIFORM_BUFFER, // change the contents of the buffer
							0, // offset from the beginning of the buffer
							sizeof(InstanceData), // size of the changes
							static_cast<GLvoid*>(&instanceData)); // new content

			glBindBufferBase(GL_UNIFORM_BUFFER, 0, instanceDataBuffer); // sets "instanceDataBuffer" to the slot "0"
		}

		glClearColor(0, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, lenaTexture);

		Render(quadVAO);

		SwapBuffers(s_WindowHandleToDeviceContext);

		{
			LARGE_INTEGER l_CurrentTime;
			QueryPerformanceCounter(&l_CurrentTime);
			//float Result = ((float)(l_CurrentTime.QuadPart - l_LastFrameTime.QuadPart) / (float)l_PerfCountFrequency);

			if (l_LastFrameTime.QuadPart + l_TicksPerFrame > l_CurrentTime.QuadPart)
			{
				int64_t ticksToSleep = l_LastFrameTime.QuadPart + l_TicksPerFrame - l_CurrentTime.QuadPart;
				int64_t msToSleep = 1000 * ticksToSleep / l_PerfCountFrequency;
				if (msToSleep > 0)
				{
					Sleep(static_cast<DWORD>(msToSleep));
				}
				continue;
			}
			while (l_LastFrameTime.QuadPart + l_TicksPerFrame <= l_CurrentTime.QuadPart)
			{
				l_LastFrameTime.QuadPart += l_TicksPerFrame;
			}

			dt = static_cast<double>(l_TicksPerFrame) / static_cast<double>(l_PerfCountFrequency);
		}
	} while (!quit);

	return 0;
}