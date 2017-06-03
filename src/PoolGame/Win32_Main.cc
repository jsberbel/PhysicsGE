#include "Win32_Main.hh"
#include <map>
#include <string>
#include "Allocators.hpp"
#include "TaskManagerHelpers.hh"

// global static vars
static HGLRC   s_OpenGLRenderingContext		 { nullptr };
static HDC	   s_WindowHandleToDeviceContext { nullptr };

static bool	   s_windowActive				 { true };
static GLsizei s_screenWidth				 { 1920 }; // 900 
static GLsizei s_screenHeight				 { 1080 }; // 600
static GLsizei s_windowResized				 { false };

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

static void GLAPIENTRY
openGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
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

LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
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
		Assert(glewInit() == GLEW_OK);

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

		if (WGLEW_EXT_swap_control)
			wglSwapIntervalEXT(1);

		// Enable blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		#ifndef _DEBUG
			//ToggleFullscreen(hWnd);
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
		glViewport(0, 0, s_screenWidth = GET_X_LPARAM(lParam), s_screenHeight = GET_Y_LPARAM(lParam));
		s_windowResized = true;
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

static constexpr bool
CompileShader(GLuint &shaderId_, const uint8_t* shaderCode, GLenum shaderType)
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

static constexpr bool
LinkShaders(GLuint &programId_, GLuint &vs, GLuint &ps)
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

template <GLuint type, GLenum usage = GL_STATIC_DRAW>
static constexpr GLuint
CreateBuffer(const void *data, GLsizei size)
{
	GLuint buffer;
	glGenBuffers(1, &buffer); // creates a buffer
	glBindBuffer(type, buffer);
	glBufferData(  // sets the buffer content
				 type,		// type of buffer
				 size,		// size of the buffer content
				 data,		// content of the buffer
				 usage);	// usage of the buffer. DYNAMIC -> will change frequently. DRAW -> from CPU to GPU
	//glBindBuffer(type, 0); // set no buffer as the current buffer
	return buffer;
}

static inline Win32::VAO
CreateVertexArrayObject(const Win32::VertexTN * vertexs, int numVertexs, const uint16_t * indices, int numIndices)
{
	// GENERATE VAO INFO
	Win32::VAO vao;
	vao.numIndices = numIndices;
	glGenVertexArrays(1, &vao.vao);
	glBindVertexArray(vao.vao);
	vao.vbo = CreateBuffer<GL_ARRAY_BUFFER>(vertexs, sizeof(Win32::VertexTN) * numVertexs);  // create an array buffer (vertex buffer)
	vao.ebo = CreateBuffer<GL_ELEMENT_ARRAY_BUFFER>(indices, sizeof(uint16_t) * numIndices); // create an element array buffer (index buffer)

	// VERTEX POSITION DATA
	glEnableVertexAttribArray(Win32::Renderer::InputLocation::POSITION);
	glVertexAttribPointer(Win32::Renderer::InputLocation::POSITION, // Vertex Attrib Index
		3, GL_FLOAT, // 3 floats
		GL_FALSE, // no normalization
		sizeof(Win32::VertexTN), // offset from a vertex to the next
		reinterpret_cast<GLvoid*>(offsetof(Win32::VertexTN, p)) // offset from the start of the buffer to the first vertex
	); // positions

	// VERTEX UV DATA
	glEnableVertexAttribArray(Win32::Renderer::InputLocation::TEXCOORD);
	glVertexAttribPointer(Win32::Renderer::InputLocation::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(Win32::VertexTN),
						  reinterpret_cast<GLvoid*>(offsetof(Win32::VertexTN, t))); // textures
	//glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Win32::VertexTN), reinterpret_cast<GLvoid*>(offsetof(Win32::VertexTN, c))); // colors
	//glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTN), reinterpret_cast<GLvoid*>(offsetof(VertexTN, n))); // normals

	// INSTANCING DATA
	for (int i = 0; i < 4; ++i)
	{
		glEnableVertexAttribArray(Win32::Renderer::InputLocation::MODELMAT + i);
		glVertexAttribPointer(Win32::Renderer::InputLocation::MODELMAT + i, 4, GL_FLOAT, GL_FALSE, sizeof glm::mat4, (const GLvoid*)(sizeof(GLfloat) * i * 4));
		glVertexAttribDivisor(Win32::Renderer::InputLocation::MODELMAT + i, 1);
	}

	// reset default opengl state
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	for (int i = 0; i < 6; ++i)
		glDisableVertexAttribArray(i);

	return vao;
}

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

static inline GLuint
LoadTexture(const char* path)
{
	GLuint result;
	int x, y, comp;
	unsigned char *data = stbi_load(path, &x, &y, &comp, 4);

	glGenTextures(1, &result);
	glBindTexture(GL_TEXTURE_2D, result);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return result;
}

static constexpr bool
HandleMouse(const MSG& msg, Game::InputData &data_)
{
	switch (msg.message)
	{
		case WM_LBUTTONDOWN:
			data_.mouseButtonL = data_.mouseButtonL == Game::InputData::ButtonState::DOWN ? Game::InputData::ButtonState::HOLD : Game::InputData::ButtonState::DOWN;
			return true;
		case WM_LBUTTONUP:
			data_.mouseButtonL = Game::InputData::ButtonState::UP;
			return true;
		case WM_RBUTTONDOWN:
			data_.mouseButtonR = data_.mouseButtonR == Game::InputData::ButtonState::DOWN ? Game::InputData::ButtonState::HOLD : Game::InputData::ButtonState::DOWN;
			return true;
		case WM_RBUTTONUP:
			data_.mouseButtonR = Game::InputData::ButtonState::UP;
			return true;
		case WM_MOUSEMOVE:
			data_.mousePosition.x = GET_X_LPARAM(msg.lParam);
			data_.mousePosition.y = GET_Y_LPARAM(msg.lParam);
			return true;
		case WM_MOUSEWHEEL:
			data_.mouseWheelZoom = GET_WHEEL_DELTA_WPARAM(msg.wParam)*0.001f;
			return true;
		default:
			return false;
	}
}

static inline void
RenderDearImgui(const Win32::Renderer &renderer)
{
	ImDrawData* draw_data = ImGui::GetDrawData(); // instructions list to draw

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	ImGuiIO& io = ImGui::GetIO();
	int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	if (fb_width == 0 || fb_height == 0/* || draw_data == nullptr*/)
		return;
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	// We are using the OpenGL fixed pipeline to make the example code simpler to read!
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);

	GLuint program = renderer.programs[Win32::Renderer::DearImgui];
	glUseProgram(program);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, renderer.uniforms[Win32::Renderer::DearImgui]);
	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context

	// Setup viewport, orthographic projection matrix
	glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);

	glBindBuffer(GL_UNIFORM_BUFFER, renderer.uniforms[Win32::Renderer::DearImgui]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ImVec2), (GLvoid*)&io.DisplaySize);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	auto texture = renderer.textures[static_cast<int>(Game::RenderData::TextureID::DEARIMGUI)];

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(renderer.vaos[Win32::Renderer::DearImgui].vao);
	glBindBuffer(GL_ARRAY_BUFFER, renderer.vaos[Win32::Renderer::DearImgui].vbo);

	// Render command lists
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		const ImDrawVert* vtx_buffer = &cmd_list->VtxBuffer.front();
		const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ImDrawVert) * cmd_list->VtxBuffer.size(), (GLvoid*)vtx_buffer);

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				if (pcmd->TextureId == nullptr)
					glBindTexture(GL_TEXTURE_2D, texture);
				else
					glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);

				glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				static_assert(sizeof(ImDrawIdx) == 2 || sizeof(ImDrawIdx) == 4, "indexes are 2 or 4 bytes long");
				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
			}
			idx_buffer += pcmd->ElemCount;
		}
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Restore modified state
	glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
	glDisable(GL_SCISSOR_TEST);
	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
}

void Win32::GenerateWindow(HINSTANCE hInstance, const wchar_t *name)
{
	WNDCLASS wc { 0 }; // Contains the window class attributes
	wc.lpfnWndProc = WndProc; // A pointer to the window procedure (fn that processes msgs)
	wc.hInstance = hInstance; // A handle to the instance that contains the window procedure
	wc.hbrBackground = HBRUSH(COLOR_BACKGROUND); // A handle to the class background brush
	wc.lpszClassName = name; // Specifies the window class name
	wc.style = CS_OWNDC; // Class styles: CS_OWNDC -> Allocates a unique device context for each window in the class

	Assert(RegisterClass(&wc)); // Registers a window class for subsequent use in calls
	HWND hWnd { CreateWindowW(wc.lpszClassName, name, // Class name initialized before // Window name
							  WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Style of the window created
							  CW_USEDEFAULT, CW_USEDEFAULT, // Window position in device units
							  s_screenWidth, s_screenHeight, // Window size in device units
							  nullptr, nullptr, // Parent handle // Menu handle
							  hInstance, nullptr) }; // Handle to instance of the module // Pointer to winstuff
}

#include "IO.hh"

GLuint Win32::GenerateProgram(const wchar_t *vertexShader, const wchar_t *fragmentShader)
{
	GLuint vs, ps, program;
	wchar_t vertexPath[50];
	wchar_t fragmentPath[50];
	wsprintf(vertexPath, L"../../res/shaders/%s", vertexShader);
	wsprintf(fragmentPath, L"../../res/shaders/%s", fragmentShader);
	Assert(CompileShader(vs, ReadFullFile(vertexPath).data, GL_VERTEX_SHADER) &&
		   CompileShader(ps, ReadFullFile(fragmentPath).data, GL_FRAGMENT_SHADER) &&
		   LinkShaders(program, vs, ps));

	if (!vs || vs == GL_INVALID_ENUM)
		glDeleteShader(vs);
	if (!ps || ps == GL_INVALID_ENUM)
		glDeleteShader(ps);

	return program;
}

//int _tmain(int argc, _TCHAR* argv[])
int __stdcall
WinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd)
{
	// INIT TIMER
	LARGE_INTEGER l_LastFrameTime;
	int64_t l_PerfCountFrequency;
	{
		LARGE_INTEGER PerfCountFrequencyResult;
		QueryPerformanceFrequency(&PerfCountFrequencyResult);
		l_PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	}
	int64_t l_TicksPerFrame = l_PerfCountFrequency / Game::MaxFPS;

	// LOAD WINDOW STUFF
	Win32::GenerateWindow(hInstance, L"Pool Game");

	// INIT SHADERS
	Win32::Renderer renderer;
	renderer.programs[Win32::Renderer::GameScene] = Win32::GenerateProgram(L"Simple.vs", L"Simple.fs");
	renderer.programs[Win32::Renderer::DearImgui] = Win32::GenerateProgram(L"DearImgui.vert", L"DearImgui.frag");

	Game::RenderData renderData;
	renderData.texture = Game::RenderData::TextureID::BALL_WHITE;

	GLuint instanceModelBuffer;
	glGenBuffers(1, &instanceModelBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, instanceModelBuffer);
	glBufferData(GL_ARRAY_BUFFER, Game::MaxGameObjects * sizeof glm::mat4, &renderData.modelMatrices[0], GL_DYNAMIC_DRAW);

	GLuint colorBuffer;
	glGenBuffers(1, &colorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, Game::MaxGameObjects * sizeof glm::vec4, &renderData.colors[0], GL_DYNAMIC_DRAW);

	// UNIFORMS
	glGenBuffers(Win32::Renderer::UNIFORM_COUNT, renderer.uniforms);
	{
		glBindBuffer(GL_UNIFORM_BUFFER, renderer.uniforms[Win32::Renderer::GameScene]);
		glBufferData(  // sets the buffer content
					 GL_UNIFORM_BUFFER,		// type of buffer
					 sizeof Win32::InstanceData,		// size of the buffer content
					 nullptr,		// content of the buffer
					 GL_DYNAMIC_DRAW);	// usage of the buffer. DYNAMIC -> will change frequently. DRAW -> from CPU to GPU

		glBindBuffer(GL_UNIFORM_BUFFER, renderer.uniforms[Win32::Renderer::DearImgui]);
		glBufferData(  // sets the buffer content
					 GL_UNIFORM_BUFFER,		// type of buffer
					 sizeof ImVec2,		// size of the buffer content
					 nullptr,		// content of the buffer
					 GL_DYNAMIC_DRAW);	// usage of the buffer. DYNAMIC -> will change frequently. DRAW -> from CPU to GPU
	}

	// GAME VAO
	Win32::VertexTN vtxs[]{
		{ glm::vec3 { -1, -1, 0 }, glm::vec3 { 0, 0, 0 }, glm::vec4 { 1, 1, 1, 1 }, glm::vec2 { 0,0 } },
		{ glm::vec3 { +1, -1, 0 }, glm::vec3 { 0, 0, 0 }, glm::vec4 { 1, 1, 1, 1 }, glm::vec2 { 1,0 } },
		{ glm::vec3 { -1, +1, 0 }, glm::vec3 { 0, 0, 0 }, glm::vec4 { 1, 1, 1, 1 }, glm::vec2 { 0,1 } },
		{ glm::vec3 { +1, +1, 0 }, glm::vec3 { 0, 0, 0 }, glm::vec4 { 1, 1, 1, 1 }, glm::vec2 { 1,1 } }
	};
	uint16_t idxs[]{
		0, 1, 2,
		2, 1, 3
	};
	//renderer.vaos[Win32::Renderer::GameScene] = CreateVertexArrayObject(vtxs, sizeof vtxs / sizeof Win32::VertexTN, idxs, sizeof idxs / sizeof uint16_t);
	// GENERATE VAO INFO
	{
		renderer.vaos[Win32::Renderer::GameScene].numIndices = sizeof idxs / sizeof uint16_t;
		glGenVertexArrays(1, &renderer.vaos[Win32::Renderer::GameScene].vao);
		glBindVertexArray(renderer.vaos[Win32::Renderer::GameScene].vao);
		renderer.vaos[Win32::Renderer::GameScene].vbo = 
			CreateBuffer<GL_ARRAY_BUFFER>(vtxs, sizeof(Win32::VertexTN) * (sizeof vtxs / sizeof Win32::VertexTN));  // create an array buffer (vertex buffer)
		renderer.vaos[Win32::Renderer::GameScene].ebo = 
			CreateBuffer<GL_ELEMENT_ARRAY_BUFFER>(idxs, sizeof(uint16_t) * (sizeof idxs / sizeof uint16_t)); // create an element array buffer (index buffer)
		
		// VERTEX POSITION DATA
		glEnableVertexAttribArray(Win32::Renderer::InputLocation::POSITION);
		glVertexAttribPointer(Win32::Renderer::InputLocation::POSITION, // Vertex Attrib Index
							  3, GL_FLOAT, // 3 floats
							  GL_FALSE, // no normalization
							  sizeof(Win32::VertexTN), // offset from a vertex to the next
							  reinterpret_cast<GLvoid*>(offsetof(Win32::VertexTN, p)) // offset from the start of the buffer to the first vertex
		);

		// VERTEX UV DATA
		glEnableVertexAttribArray(Win32::Renderer::InputLocation::TEXCOORD);
		glVertexAttribPointer(Win32::Renderer::InputLocation::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(Win32::VertexTN),
							  reinterpret_cast<GLvoid*>(offsetof(Win32::VertexTN, t)));

		// VERTEX COLOR DATA
		glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
		glEnableVertexAttribArray(Win32::Renderer::InputLocation::COLOR);
		glVertexAttribPointer(Win32::Renderer::InputLocation::COLOR, 4, GL_FLOAT, GL_FALSE, sizeof glm::vec4,
								(const GLvoid*)(sizeof(GLfloat) * 0));
		glVertexAttribDivisor(Win32::Renderer::InputLocation::COLOR, 1);
		//glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTN), reinterpret_cast<GLvoid*>(offsetof(VertexTN, n))); // normals

		// INSTANCING DATA
		glBindBuffer(GL_ARRAY_BUFFER, instanceModelBuffer);
		for (int i = 0; i < 4; ++i)
		{
			glEnableVertexAttribArray(Win32::Renderer::InputLocation::MODELMAT + i);
			glVertexAttribPointer(Win32::Renderer::InputLocation::MODELMAT + i, 4, GL_FLOAT, GL_FALSE, sizeof glm::mat4, (const GLvoid*)(sizeof(GLfloat) * i * 4));
			glVertexAttribDivisor(Win32::Renderer::InputLocation::MODELMAT + i, 1);
		}

		// reset default opengl state
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		for (int i = 0; i < 6; ++i)
			glDisableVertexAttribArray(i);
	}

	// IMGUI VAO
	{
		glGenVertexArrays(1, &renderer.vaos[Win32::Renderer::DearImgui].vao);

		renderer.vaos[Win32::Renderer::DearImgui].vbo = CreateBuffer<GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW>(nullptr, sizeof(ImDrawVert) * Win32::Renderer::BuffersSize);  // create an array buffer (vertex buffer)
		//renderer.vaos[Win32::Renderer::DearImgui].ebo = CreateBuffer<GL_ELEMENT_ARRAY_BUFFER>(indices, sizeof(uint16_t) * numIndices); // create an element array buffer (index buffer)

		//glGenBuffers(Win32::Renderer::VAO_COUNT/*VERTEX_ARRAY_COUNT*/, &renderer.vaos[Win32::Renderer::DearImgui].vbo);
		//glGenVertexArrays(Win32::Renderer::VAO_COUNT, &renderer.vaos[Win32::Renderer::DearImgui].vao);

		glBindVertexArray(renderer.vaos[Win32::Renderer::DearImgui].vao);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, pos));
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, uv));
		glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, col));

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
	
	// LOAD TEXTURES
	glGenTextures(Win32::Renderer::TEXTURE_COUNT, renderer.textures);
	{
		/*std::string texturePath;
		for (int i = static_cast<int>(Game::RenderData::TextureID::BALL_WHITE);
			     i < static_cast<int>(Game::RenderData::TextureID::MAX_BALLS);
			     ++i)
		{
			texturePath = "../../res/images/ball" + std::to_string(i) + ".png";
			renderer.textures[i] = LoadTexture(texturePath.c_str());
		}*/
		renderer.textures[static_cast<int>(Game::RenderData::TextureID::BALL_WHITE)] = LoadTexture("../../res/images/ball14.png");
		renderer.textures[static_cast<int>(Game::RenderData::TextureID::PIXEL)] = LoadTexture("../../res/images/pixel.png");

		// TODO reload imgui
		uint8_t* pixels;

		int width, height;
		ImGui::GetIO().Fonts[0].GetTexDataAsRGBA32(&pixels, &width, &height, nullptr);

		glBindTexture(GL_TEXTURE_2D, renderer.textures[static_cast<int>(Game::RenderData::TextureID::DEARIMGUI)]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		GLenum formats[4] = { GL_RED, GL_RG, GL_RGB, GL_RGBA };

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	}

	// KEYBOARD MAPPING
	bool buttonsPrevKey[255], buttonsNowKey[255];
	memset(buttonsPrevKey, false, 255);
	memset(buttonsNowKey, false, 255);
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)s_screenWidth, (float)s_screenHeight);
	{
		io.KeyMap[ImGuiKey_Tab] = VK_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
		io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
		io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
		io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
		io.KeyMap[ImGuiKey_Home] = VK_HOME;
		io.KeyMap[ImGuiKey_End] = VK_END;
		io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
		io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
		io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
		io.KeyMap[ImGuiKey_A] = 'A';
		io.KeyMap[ImGuiKey_C] = 'C';
		io.KeyMap[ImGuiKey_V] = 'V';
		io.KeyMap[ImGuiKey_X] = 'X';
		io.KeyMap[ImGuiKey_Y] = 'Y';
		io.KeyMap[ImGuiKey_Z] = 'Z';
	}

	// GAME DATA
	Game::InputData inputData;
	inputData.windowHalfSize = { s_screenWidth / 2, s_screenHeight / 2 };
	Game::GameData * gameData = Game::InitGamedata(inputData);

	// create a orthogonal projection matrix
	glm::mat4 projection = glm::ortho(-static_cast<float>(inputData.windowHalfSize.x), // left
									  static_cast<float>(inputData.windowHalfSize.x), // right
									  -static_cast<float>(inputData.windowHalfSize.y), // bot
									  static_cast<float>(inputData.windowHalfSize.y), // top
									  -5.0f, 5.0f); // near // far

	// TASK MANAGER
	int numThreads;
	{
		// init worker threads
		unsigned int numHardwareCores = std::thread::hardware_concurrency();
		numHardwareCores = numHardwareCores > 1 ? numHardwareCores - 1 : 1;
		numThreads = (numHardwareCores < Utilities::Profiler::MaxNumThreads - 1) ? numHardwareCores : Utilities::Profiler::MaxNumThreads - 1;
		std::thread *workerThread = (std::thread*)alloca(sizeof(std::thread) * numThreads);

		size_t totalMemory2 = 1 * 1024 * 1024 * 1024; // 1 GB :DDDDDDDDDDD
		void* baseAddress2 = reinterpret_cast<void*>(0x30000000000);
		uint8_t* gameMemoryBlock2 = reinterpret_cast<uint8_t*>(VirtualAlloc(baseAddress2,
															   totalMemory2,
															   MEM_RESERVE | MEM_COMMIT,
															   PAGE_READWRITE));

		Utilities::DefaultAllocator blockAllocator(gameMemoryBlock2, totalMemory2);

		Utilities::TaskManager::JobContext mainThreadContext;// {nullptr, &blockAllocator, numThreads, -1};
		mainThreadContext.scheduler = nullptr;
		mainThreadContext.profiler = &Win32::s_Profiler;
		mainThreadContext.allocator = &blockAllocator;
		mainThreadContext.threadIndex = numThreads;
		mainThreadContext.fiberIndex = -1;

		Win32::s_JobScheduler.Init(numThreads, &Win32::s_Profiler, &blockAllocator);

		for (int i = 0; i < numThreads; ++i)
		{
			new(&workerThread[i]) std::thread(Win32::WorkerThread, i);
		}
	}
	std::mutex frameLockMutex;
	std::condition_variable frameLockConditionVariable;

	// INIT TIME
	QueryPerformanceCounter(&l_LastFrameTime);

	struct Camera
	{
		glm::ivec2 pos{ 0, 0 };
		float zoom{ 1.f };
		bool needsUpdate = false;
	} camera;

	bool quit{ false };
	do {
		// PROCESS MESSAGES
		{
			MSG msg{};
			inputData.mouseWheelZoom = false;

			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				bool fHandled = false;
				if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
					fHandled = HandleMouse(msg, inputData);
				else if (msg.message == WM_QUIT)
					quit = fHandled = true;
				else if (msg.message == WM_KEYDOWN)
				{
					buttonsNowKey[msg.wParam & 255] = true;
					fHandled = true;
					if (msg.wParam == VK_ESCAPE)
						PostQuitMessage(0);
				}
				else if (msg.message == WM_KEYUP)
				{
					buttonsNowKey[msg.wParam & 255] = false;
					fHandled = true;
				}
				if (msg.message == WM_MOUSEWHEEL)
				{
					inputData.isZooming = true;
					inputData.mouseWheelZoom = GET_Y_LPARAM(msg.wParam);
					constexpr auto maxZoom = 0.f;
					camera.zoom = camera.zoom >= maxZoom ? camera.zoom - (camera.zoom*inputData.mouseWheelZoom)/1000 : maxZoom;
					camera.needsUpdate = true;
				}

				HandleMouse(msg, inputData);

				if (!fHandled)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}

			if (s_windowResized)
			{
				inputData.windowHalfSize = { s_screenWidth / 2, s_screenHeight / 2 };
				projection = glm::ortho(-static_cast<float>(inputData.windowHalfSize.x), // left
					static_cast<float>(inputData.windowHalfSize.x), // right
					-static_cast<float>(inputData.windowHalfSize.y), // bot
					static_cast<float>(inputData.windowHalfSize.y), // top
					-5.0f, 5.0f); // near // far
				io.DisplaySize = ImVec2((float)s_screenWidth, (float)s_screenHeight);
				s_windowResized = false;
			}

			// MOVE CAMERA
			constexpr auto speed = 600.f;
			if (buttonsNowKey[0x41]) // A
			{
				camera.pos.x -= speed*inputData.dt*(camera.zoom * 2);
				camera.needsUpdate = true;
			}
			if (buttonsNowKey[0x44]) // D
			{
				camera.pos.x += speed*inputData.dt*(camera.zoom * 2);
				camera.needsUpdate = true;
			}
			if (buttonsNowKey[0x53]) // S
			{
				camera.pos.y -= speed*inputData.dt*(camera.zoom * 2);
				camera.needsUpdate = true;
			}
			if (buttonsNowKey[0x57]) // W
			{
				camera.pos.y += speed*inputData.dt*(camera.zoom * 2);
				camera.needsUpdate = true;
			}

			if (camera.needsUpdate)
			{
				projection = glm::ortho(-static_cast<float>((-camera.pos.x+ inputData.windowHalfSize.x * camera.zoom)), // left
										static_cast<float>((camera.pos.x + inputData.windowHalfSize.x * camera.zoom)), // right
										-static_cast<float>((-camera.pos.y + inputData.windowHalfSize.y * camera.zoom)), // bot
										static_cast<float>((camera.pos.y + inputData.windowHalfSize.y * camera.zoom)), // top
										-5.0f, 5.0f); // near // far
				camera.needsUpdate = false;
			}
		}

		LARGE_INTEGER l_CurrentTime;
		QueryPerformanceCounter(&l_CurrentTime);
		//float Result = ((float)(l_CurrentTime.QuadPart - l_LastFrameTime.QuadPart) / (float)l_PerfCountFrequency);

		if (l_LastFrameTime.QuadPart + l_TicksPerFrame > l_CurrentTime.QuadPart)
		{
			int64_t ticksToSleep = l_LastFrameTime.QuadPart + l_TicksPerFrame - l_CurrentTime.QuadPart;
			int64_t msToSleep = 1000 * ticksToSleep / l_PerfCountFrequency;
			if (msToSleep > 0)
				Sleep(static_cast<DWORD>(msToSleep));
			continue;
		}

		int numFramesElapsed = 0;
		while (l_LastFrameTime.QuadPart + l_TicksPerFrame <= l_CurrentTime.QuadPart)
		{
			l_LastFrameTime.QuadPart += l_TicksPerFrame;
			++numFramesElapsed;
		}

		bool hasFinishedUpdating = false;
		auto updateJob = Utilities::TaskManager::CreateLambdaJob([&](int, const Utilities::TaskManager::JobContext &context)
		{
			for (int i = 0; i < numFramesElapsed; ++i)
			{
				inputData.dt = static_cast<double>(l_TicksPerFrame) / static_cast<double>(l_PerfCountFrequency);
				
				// UPDATE
				Update(*gameData, renderData, inputData, context);

				LARGE_INTEGER l_UpdateTime;
				QueryPerformanceCounter(&l_UpdateTime);
				int64_t updateTicks = l_UpdateTime.QuadPart - l_CurrentTime.QuadPart;
				double ticksPerUpdate = (double)updateTicks / (i + 1);
				if (ticksPerUpdate * (i + 2) > l_TicksPerFrame)
					break;
			}
			{
				std::unique_lock<std::mutex> lock(frameLockMutex);
				hasFinishedUpdating = true;
			}
			frameLockConditionVariable.notify_all();
		}, "Game Update", 1, 0, Utilities::TaskManager::Job::Priority::HIGH, true);

		Win32::s_JobScheduler.Do(&updateJob, nullptr);

		{
			std::unique_lock<std::mutex> lock(frameLockMutex);

			while (!hasFinishedUpdating)
				frameLockConditionVariable.wait(lock);
		}

		// IMGUI
		{
			io.DeltaTime = float(inputData.dt);
			io.MouseDown[0] = inputData.mouseButtonL == Game::InputData::ButtonState::DOWN || inputData.mouseButtonL == Game::InputData::ButtonState::HOLD;
			io.MouseDown[1] = inputData.mouseButtonR == Game::InputData::ButtonState::DOWN || inputData.mouseButtonR == Game::InputData::ButtonState::HOLD;
			io.MousePos = ImVec2(float(inputData.mousePosition.x), float(inputData.mousePosition.y));
			ImGui::NewFrame();

			/*for (int k = 0; k < 255; ++k)
				buttonsPrevKey[k] = buttonsNowKey[k];*/
		}

		glClearColor(0.f, 0.0f, 0.0f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(renderer.programs[Win32::Renderer::GameScene]);
		glActiveTexture(GL_TEXTURE0 + 0);

		Win32::s_Profiler.AddProfileMark(Utilities::Profiler::MarkerType::BEGIN, nullptr, "Instanced Rendering");
		{
			glBindTexture(GL_TEXTURE_2D, renderer.textures[static_cast<int>(Game::RenderData::TextureID::BALL_WHITE)]); // get the right texture

			Win32::InstanceData instanceData{ projection };
			glBindBuffer(GL_UNIFORM_BUFFER, renderer.uniforms[Win32::Renderer::GameScene]);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Win32::InstanceData), static_cast<GLvoid*>(&instanceData));
			glBindBufferBase(GL_UNIFORM_BUFFER, 0, renderer.uniforms[Win32::Renderer::GameScene]);

			glBindBuffer(GL_ARRAY_BUFFER, instanceModelBuffer);
			glBufferData(GL_ARRAY_BUFFER, Game::MaxGameObjects * sizeof glm::mat4, &renderData.modelMatrices[0], GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
			glBufferData(GL_ARRAY_BUFFER, Game::MaxGameObjects * sizeof glm::vec4, &renderData.colors[0], GL_DYNAMIC_DRAW);

			glBindVertexArray(renderer.vaos[Win32::Renderer::GameScene].vao);
			glDrawElementsInstanced(GL_TRIANGLES, renderer.vaos[Win32::Renderer::GameScene].numIndices,
									GL_UNSIGNED_SHORT, nullptr, Game::MaxGameObjects);
			glBindVertexArray(0);
		}
		Win32::s_Profiler.AddProfileMark(Utilities::Profiler::MarkerType::END, nullptr, "Instanced Rendering");
		//-----------------------------------------------------------------------------------------------------------------------------------
		//for (int i = 0; i < Game::MaxGameObjects; ++i)
		//{
		//	glBindTexture(GL_TEXTURE_2D, renderer.textures[static_cast<int>(renderData.sprites[i].texture)]); // get the right texture

		//	// create the model matrix, with a scale and a translation.
		//	/*glm::mat4 model{
		//		glm::scale(
		//			glm::rotate(
		//				glm::translate(glm::mat4(), glm::vec3(sprite.position, 0.f)),
		//			sprite.rotation, glm::vec3(0.f, 0.f, 1.f)),
		//		glm::vec3(sprite.size, 1.f))
		//	};*/

		//	// the transform is the addition of the model transformation and the projection
		//	Win32::InstanceData instanceData{ projection * renderData.modelMatrices[i] };

		//	glBindBuffer(GL_UNIFORM_BUFFER, renderer.uniforms[0]);
		//	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Win32::InstanceData), static_cast<GLvoid*>(&instanceData));

		//	glBindBufferBase(GL_UNIFORM_BUFFER, 0, renderer.uniforms[0]);

		//	glBindVertexArray(renderer.vaos[0].vao);
		//	glDrawElements(GL_TRIANGLES, renderer.vaos[0].numIndices, GL_UNSIGNED_SHORT, nullptr);
		//}

		Win32::s_Profiler.DrawProfilerToImGUI(numThreads);

		// RENDER IMGUI
		ImGui::SetNextWindowPos(ImVec2(inputData.windowHalfSize.x, inputData.windowHalfSize.y), ImGuiSetCond_FirstUseEver);
		//ImGui::ShowTestWindow();

		ImGui::Render();
		RenderDearImgui(renderer);
		
		SwapBuffers(s_WindowHandleToDeviceContext);

	} while (!quit);

	Game::FinalizeGameData(gameData);

	return 0;
}