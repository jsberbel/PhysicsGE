#include "Win32_Main.hh"
#include <map>

// global static vars
global_var HGLRC   s_OpenGLRenderingContext		 { nullptr };
global_var HDC	   s_WindowHandleToDeviceContext { nullptr };

global_var bool	   s_windowActive				 { true };
global_var GLsizei s_screenWidth				 { 1920 };
global_var GLsizei s_screenHeight				 { 1080 };

internal_fn void GLAPIENTRY
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

internal_fn bool
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

internal_fn bool
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
internal_fn GLuint
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

internal_fn VAO
CreateVertexArrayObject(const VertexTN * vertexs, int numVertexs, const uint16_t * indices, int numIndices)
{
	VAO vao;
	vao.numIndices = numIndices;

	glGenVertexArrays(1, &vao.vao);
	glBindVertexArray(vao.vao);

	vao.vbo = CreateBuffer<GL_ARRAY_BUFFER>(vertexs, sizeof(VertexTN) * numVertexs);  // create an array buffer (vertex buffer)
	vao.ebo = CreateBuffer<GL_ELEMENT_ARRAY_BUFFER>(indices, sizeof(uint16_t) * numIndices); // create an element array buffer (index buffer)

	glVertexAttribPointer(0, // Vertex Attrib Index
		3, GL_FLOAT, // 3 floats
		GL_FALSE, // no normalization
		sizeof(VertexTN), // offset from a vertex to the next
		reinterpret_cast<GLvoid*>(offsetof(VertexTN, p)) // offset from the start of the buffer to the first vertex
	); // positions
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexTN), reinterpret_cast<GLvoid*>(offsetof(VertexTN, c))); // colors
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexTN), reinterpret_cast<GLvoid*>(offsetof(VertexTN, t))); // textures
	//glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexTN), reinterpret_cast<GLvoid*>(offsetof(VertexTN, n))); // normals

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

	return vao;
}

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

internal_fn GLuint
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

	return result;
}

#include "IO.hh"

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
	int64_t l_TicksPerFrame = l_PerfCountFrequency / MAX_FPS;

	// LOAD WINDOW STUFF
	{
		WNDCLASS wc{ 0 }; // Contains the window class attributes
		wc.lpfnWndProc = WndProc; // A pointer to the window procedure (fn that processes msgs)
		wc.hInstance = hInstance; // A handle to the instance that contains the window procedure
		wc.hbrBackground = HBRUSH(COLOR_BACKGROUND); // A handle to the class background brush
		wc.lpszClassName = L"Asteroids"; // Specifies the window class name
		wc.style = CS_OWNDC; // Class styles: CS_OWNDC -> Allocates a unique device context for each window in the class

		Assert (RegisterClass(&wc)); // Registers a window class for subsequent use in calls
		HWND hWnd{ CreateWindowW(wc.lpszClassName, L"Asteroids", // Class name initialized before // Window name
								 WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Style of the window created
								 CW_USEDEFAULT, CW_USEDEFAULT, // Window position in device units
								 s_screenWidth, s_screenHeight, // Window size in device units
								 nullptr, nullptr, // Parent handle // Menu handle
								 hInstance, nullptr) }; // Handle to instance of the module // Pointer to winstuff
	}

	// INIT SHADERS
	GLData glData;
	{
		auto fileVS{ ReadFullFile(L"../../res/shaders/Simple.vs") };
		auto filePS{ ReadFullFile(L"../../res/shaders/Simple.fs") };
		
		GLuint vs, ps;
		Assert(CompileShader(vs, fileVS.data, GL_VERTEX_SHADER) &&
			   CompileShader(ps, filePS.data, GL_FRAGMENT_SHADER) &&
			   LinkShaders(glData.program, vs, ps));

		if (vs > 0)
			glDeleteShader(vs);
		if (ps > 0)
			glDeleteShader(ps);

		glUseProgram(glData.program); // Installs a program object as part of current rendering state
	}

	// INIT TRIANGLE VERTICES
	/*{
		VertexTN vtxs[]{
			{ glm::vec3{ -1, -1, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec4{ 1, 1, 1, 1 }, glm::vec2{ 0,0 } },
			{ glm::vec3{ -1, +1, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec4{ 1, 1, 1, 1 }, glm::vec2{ 1,0 } },
			{ glm::vec3{ +1, +0, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec4{ 1, 1, 1, 1 }, glm::vec2{ 0,1 } }
		};
		uint16_t idxs[]{
			0, 1, 2,
		};

		VAO triangleVAO = CreateVertexArrayObject(vtxs, sizeof(vtxs), idxs, sizeof(idxs));
	}*/

	// INIT QUAD VERTICES
	{
		VertexTN vtxs[]{
			{ glm::vec3{ -1, -1, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec4{ 1, 1, 1, 1 }, glm::vec2{ 0,0 } },
			{ glm::vec3{ +1, -1, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec4{ 1, 1, 1, 1 }, glm::vec2{ 1,0 } },
			{ glm::vec3{ -1, +1, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec4{ 1, 1, 1, 1 }, glm::vec2{ 0,1 } },
			{ glm::vec3{ +1, +1, 0 }, glm::vec3{ 0, 0, 0 }, glm::vec4{ 1, 1, 1, 1 }, glm::vec2{ 1,1 } }
		};
		uint16_t idxs[]{
			0, 1, 2,
			2, 1, 3
		};

		glData.quadVAO = CreateVertexArrayObject(vtxs, sizeof vtxs / sizeof VertexTN, idxs, sizeof idxs / sizeof uint16_t);
	}
	
	// INIT INSTANCE DATA BUFFER
	glData.instanceDataBuffer = CreateBuffer<GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW>(nullptr, sizeof InstanceData);

	// LOAD TEXTURES
	glData.textures[static_cast<int>(Game::RenderData::TextureID::PIXEL)] =		LoadTexture("../../res/images/pixel.png");
	glData.textures[static_cast<int>(Game::RenderData::TextureID::SPACESHIP)] = LoadTexture("../../res/images/spaceship.png");
	glData.textures[static_cast<int>(Game::RenderData::TextureID::ASTEROID)] =  LoadTexture("../../res/images/asteroid.png");

	// GAME DATA
	Game::InputData inputData;
	inputData.windowHalfSize = { s_screenWidth / 2, s_screenHeight / 2 };
	Game::GameData * gameData = Game::InitGamedata(inputData);

	bool buttonsPrevKey[255], buttonsNowKey[255];
	memset(buttonsPrevKey, false, 255);
	memset(buttonsNowKey, false, 255);

	// create a orthogonal projection matrix¡
	glm::mat4 projection = glm::ortho(-static_cast<float>(inputData.windowHalfSize.x), //left
									  static_cast<float>(inputData.windowHalfSize.x), //right
									  -static_cast<float>(inputData.windowHalfSize.y), //bot
									  static_cast<float>(inputData.windowHalfSize.y), //top
									  -5.0f, 5.0f); // near // far

	// TEXT RENDERING
	GLData glyphData;
	std::map<GLchar, Character> characters;
	{
		auto fileVS { ReadFullFile(L"../../res/shaders/glyph.vs") };
		auto filePS { ReadFullFile(L"../../res/shaders/glyph.fs") };
		GLuint vs, ps;
		Assert(CompileShader(vs, fileVS.data, GL_VERTEX_SHADER) &&
			   CompileShader(ps, filePS.data, GL_FRAGMENT_SHADER) &&
			   LinkShaders(glyphData.program, vs, ps));
		glUseProgram(glyphData.program);
		glUniformMatrix4fv(glGetUniformLocation(glyphData.program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		FT_Library ft;
		Assert (!FT_Init_FreeType(&ft));
		FT_Face face;
		Assert (!FT_New_Face(ft, "../../res/fonts/arial.ttf", 0, &face));

		FT_Set_Pixel_Sizes(face, 0, 48);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

		for (GLubyte c = 0; c < 128; c++)
		{
			// Load character glyph 
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				OutputDebugStringA("ERROR::FREETYTPE: Failed to load Glyph");
				continue;
			}
			// Generate texture
			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
			);
			// Set texture options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// Now store character for later use
			Character character = {
				texture,
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				static_cast<GLuint>(face->glyph->advance.x)
			};
			characters.insert(std::pair<GLchar, Character>(c, character));
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		FT_Done_Face(face);
		FT_Done_FreeType(ft);

		glGenVertexArrays(1, &glyphData.quadVAO.vao);
		glGenBuffers(1, &glyphData.quadVAO.vbo);
		glBindVertexArray(glyphData.quadVAO.vao);
		glBindBuffer(GL_ARRAY_BUFFER, glyphData.quadVAO.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	// INIT TIME
	QueryPerformanceCounter(&l_LastFrameTime);

	bool quit{ false };
	do {
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
		
		int numFramesElapsed = 0;
		while (l_LastFrameTime.QuadPart + l_TicksPerFrame <= l_CurrentTime.QuadPart)
		{
			l_LastFrameTime.QuadPart += l_TicksPerFrame;
			++numFramesElapsed;
		}

		inputData.dt = static_cast<double>(l_TicksPerFrame) / static_cast<double>(l_PerfCountFrequency);

		/*char buffer[512];
		sprintf_s(buffer, "dt: %d\n", numFramesElapsed);
		OutputDebugStringA(buffer);*/

		Game::RenderData renderData;
		for (int i = 0; i < numFramesElapsed; ++i)
		{
			// PROCESS MESSAGES
			{
				MSG msg {};

				while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
				{
					bool processed { false };
					switch (msg.message)
					{
						case WM_QUIT:
							quit = true;
							processed = true;
							break;
						case WM_KEYDOWN:
							buttonsNowKey[msg.wParam & 255] = true;
							processed = true;
							if (msg.wParam == VK_ESCAPE)
								PostQuitMessage(0);
							break;
						case WM_KEYUP:
							buttonsNowKey[msg.wParam & 255] = false;
							processed = true;
							break;
					}

					if (!processed)
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
				
				inputData.buttonUp	  =	Game::InputData::ProcessKey(buttonsPrevKey[0x57], buttonsNowKey[0x57]);
				inputData.buttonLeft  =	Game::InputData::ProcessKey(buttonsPrevKey[0x41], buttonsNowKey[0x41]);
				inputData.buttonRight = Game::InputData::ProcessKey(buttonsPrevKey[0x44], buttonsNowKey[0x44]);
				inputData.buttonDown  =	Game::InputData::ProcessKey(buttonsPrevKey[0x53], buttonsNowKey[0x53]);
          		inputData.buttonShoot = Game::InputData::ProcessKey(buttonsPrevKey[VK_SPACE], buttonsNowKey[VK_SPACE]);

				for (auto *bpk = buttonsPrevKey, *bnk = buttonsNowKey; 
					 bpk != buttonsPrevKey + 255, bnk != buttonsNowKey + 255;
					 ++bpk, ++bnk)
				{
					*bpk = *bnk;
				}

				/*char buffer[512];
				sprintf_s(buffer, "prev: %d, now: %d\n", buttonsPrevKey[VK_SPACE], buttonsNowKey[VK_SPACE]);
				OutputDebugStringA(buffer);*/
			}

			renderData = Update(*gameData, inputData);
			
			LARGE_INTEGER l_UpdateTime;
			QueryPerformanceCounter(&l_UpdateTime);

			int64_t updateTicks = l_UpdateTime.QuadPart - l_CurrentTime.QuadPart;
			double ticksPerUpdate = double(updateTicks) / (i + 1);
			if (ticksPerUpdate * (i + 2) > l_TicksPerFrame)
				break;
		}

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(glData.program);
		glActiveTexture(GL_TEXTURE0 + 0);

		// RENDER SPRITES
		for (auto & sprite : renderData.sprites)
		{
			glBindTexture(GL_TEXTURE_2D, glData.textures[static_cast<int>(sprite.texture)]); // get the right texture

			// create the model matrix, with a scale and a translation.
			glm::mat4 model {
				glm::scale(
					glm::rotate(
						glm::translate(glm::mat4(), glm::vec3(sprite.position, 0.f)),
					sprite.rotation, glm::vec3(0.f, 0.f, 1.f)),
				glm::vec3(sprite.size, 1.f))
			};

			// the transform is the addition of the model transformation and the projection
			InstanceData instanceData { projection * model , glm::vec4(sprite.color, 1) };

			glBindBuffer(GL_UNIFORM_BUFFER, glData.instanceDataBuffer);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(InstanceData), static_cast<GLvoid*>(&instanceData));

			glBindBufferBase(GL_UNIFORM_BUFFER, 0, glData.instanceDataBuffer);

			glBindVertexArray(glData.quadVAO.vao);
			glDrawElements(GL_TRIANGLES, glData.quadVAO.numIndices, GL_UNSIGNED_SHORT, nullptr);
		}

		// RENDER TEXT
		for (auto & text : renderData.texts)
		{
			// Activate corresponding render state	
			glUseProgram(glyphData.program);
			glUniform3f(glGetUniformLocation(glyphData.program, "textColor"), text.color.x, text.color.y, text.color.z);
			glActiveTexture(GL_TEXTURE0);
			glBindVertexArray(glyphData.quadVAO.vao);

			// Iterate through all characters
			for (auto & c : text.msg)
			{
				Character ch = characters[c];

				GLfloat xpos = text.position.x + ch.bearing.x * text.scale;
				GLfloat ypos = text.position.y - (ch.size.y - ch.bearing.y) * text.scale;

				GLfloat w = ch.size.x * text.scale;
				GLfloat h = ch.size.y * text.scale;
				// Update VBO for each character
				GLfloat vertices[6][4] = {
					{ xpos,     ypos + h,   0.0, 0.0 },
					{ xpos,     ypos,       0.0, 1.0 },
					{ xpos + w, ypos,       1.0, 1.0 },

					{ xpos,     ypos + h,   0.0, 0.0 },
					{ xpos + w, ypos,       1.0, 1.0 },
					{ xpos + w, ypos + h,   1.0, 0.0 }
				};
				// Render glyph texture over quad
				glBindTexture(GL_TEXTURE_2D, ch.textureID);
				// Update content of VBO memory
				glBindBuffer(GL_ARRAY_BUFFER, glyphData.quadVAO.vbo);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				// Render quad
				glDrawArrays(GL_TRIANGLES, 0, 6);
				// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
				text.position.x += (ch.advance >> 6) * text.scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
			}
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		
		SwapBuffers(s_WindowHandleToDeviceContext);

	} while (!quit);

	Game::FinalizeGameData(gameData);

	return 0;
}