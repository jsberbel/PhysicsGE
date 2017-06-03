#pragma once

#include <Windows.h>

// opengl math library
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// opengl extension loading library
#include <GL/glew.h>
#include "GL/wglew.h" 

// imgui library
#include "imgui/imgui.h"

#include "Game.hh"
#include "Profiler.hh"
#include "TaskManager.hh"

#pragma comment (lib, "Winmm.lib")
#pragma comment (lib, "opengl32.lib")

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
	struct VertexData
	{
		glm::vec3 p;
		glm::vec2 t;
	};

	struct SceneObjectData
	{
		enum {
			VBO_Vertex,
			VBO_Color,
			VBO_InstanceModel,
			VBO_MAX
		};
		GLuint vao;
		GLuint vbo[VBO_MAX];
		GLuint ebo;
		int numIndices = 0;
	};

	struct ImGUIObjectData
	{
		GLuint vao;
		GLuint vbo;
	};

	struct Renderer
	{
		static constexpr int BuffersSize { 0x10000 };
		enum {
			GameScene = 0x00, 
			DearImgui = 0x01
		};

		enum InputLocation
		{
			POSITION, TEXCOORD, COLOR, MODELMAT
		};

		enum {
			UNIFORM_COUNT = 0x02,
			PROGRAM_COUNT = 0x02,
			TEXTURE_COUNT = static_cast<int>(Game::RenderData::TextureID::COUNT) + 0x01,
			COUNT
		};

		SceneObjectData sceneObjectData;
		ImGUIObjectData imGUIObjectData;
		GLuint uniforms[UNIFORM_COUNT];
		GLuint programs[PROGRAM_COUNT];
		GLuint textures[TEXTURE_COUNT];
	};

	struct InstanceData
	{
		glm::mat4 projection;
	};

	void GenerateWindow(HINSTANCE hInstance, const wchar_t *name);
	GLuint GenerateProgram(const wchar_t *vertexShader, const wchar_t *fragmentShader);

	// TASK MANAGER SCHEDULER
	class WinJobScheduler : public Utilities::TaskManager::JobScheduler
	{
	public:
		virtual ~WinJobScheduler() = default;

		void SwitchToFiber(void* fiber) override
		{
			::SwitchToFiber(fiber);
		}

		void* CreateFiber(size_t stackSize, void(__stdcall*call) (void*), void* parameter) override
		{
			return ::CreateFiber(stackSize, call, parameter);
		}

		void* GetFiberData() const override
		{
			return ::GetFiberData();
		}
	};

	WinJobScheduler s_JobScheduler;
	Utilities::Profiler s_Profiler;

	inline void SetThreadName(unsigned long dwThreadID, const char* threadName)
	{
		const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)  
		struct
		{
			DWORD dwType; // Must be 0x1000.  
			LPCSTR szName; // Pointer to name (in user addr space).  
			DWORD dwThreadID; // Thread ID (-1=caller thread).  
			DWORD dwFlags; // Reserved for future use, must be zero.  
		} info;
#pragma pack(pop)  

		info.dwType = 0x1000;
		info.szName = threadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;

#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
		__try {
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		}
#pragma warning(pop)  
	}

	inline void __stdcall WorkerThread(int idx)
	{
		{
			char buffer[] = { "Worker Thread " };
			_itoa_s(idx, buffer, 10);
			SetThreadName(GetCurrentThreadId(), buffer);

			//auto hr = SetThreadAffinityMask(GetCurrentThread(), 1LL << idx);
			//assert(hr != 0);
		}

		s_JobScheduler.SetRootFiber(ConvertThreadToFiber(nullptr), idx);

		s_JobScheduler.RunScheduler(idx, s_Profiler);
	}
}

