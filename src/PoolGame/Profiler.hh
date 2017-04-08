#pragma once

#include <chrono>

namespace Utilities
{
	class Profiler
	{
	public:
		enum class MarkerType : unsigned char {
			BEGIN,
			END,
			PAUSE_WAIT_FOR_JOB,
			PAUSE_WAIT_FOR_QUEUE_SPACE,
			RESUME_FROM_PAUSE,
			BEGIN_IDLE, END_IDLE,
			BEGIN_FUNCTION, END_FUNCTION,
		};

		static Profiler& instance();

		// Cridar aquesta funció cada cop que volguem registrar una marca al profiler.
		// emparellar cada BEGIN_* amb un END_* i cada PAUSE_* amb un RESUME_*. Els BEGIN_FUNCTION/END_FUNCTION han d'anar aniuats i dins de begin/end
		void AddProfileMark(MarkerType reason, const void* identifier = nullptr, const char* functionName = nullptr, int threadId = 0, int systemID = -1);

		// crea un "BEGIN_FUNCTION" i quan l'objecte creat es destrueix, un "END_FUNCTION"
		struct MarkGuard;
		MarkGuard CreateProfileMarkGuard(const char* functionName, int threadId = 0, int systemID = -1);

		// dibuixa una finestra ImGUI amb la info del darrer frame
		void DrawProfilerToImGUI(int numThreads);

		// GUARDA. S'aprofita del sistema RAII (Resource acquisition is initialization) de C++
		// i de "move semantics" de C++11 per assegurar-se que es crida el END_FUNCTION 1 vegada.
		struct MarkGuard
		{
			Profiler *profiler;
			int threadIndex;
			const void* identifier;


			MarkGuard(Profiler *_profiler, int _threadIndex, const void* _identifier) : profiler(_profiler), threadIndex(_threadIndex), identifier(_identifier) {}
			MarkGuard(const MarkGuard&) = delete;
			MarkGuard& operator=(const MarkGuard&) = delete;
			MarkGuard(MarkGuard&& other) : profiler(other.profiler), threadIndex(other.threadIndex), identifier(other.identifier) { other.profiler = nullptr; }
			MarkGuard& operator=(MarkGuard&& other) = delete;

			~MarkGuard()
			{
				if (profiler != nullptr)
				{
					profiler->AddProfileMark(MarkerType::END_FUNCTION, identifier, (const char*)nullptr, threadIndex);
				}
			}
		};

	private:

		static constexpr int ProfilerMarkerBufferSize = 16 * 1024;
		static constexpr int MaxNumThreads = 10;

		struct ProfileMarker
		{
			std::chrono::high_resolution_clock::time_point timePoint;
			const void* identifier;
			const char* jobName;
			int systemID;
			MarkerType type;

			bool IsBeginMark() const { return (type == MarkerType::BEGIN || type == MarkerType::RESUME_FROM_PAUSE || type == MarkerType::BEGIN_IDLE); }
			bool IsEndMark() const { return (type == MarkerType::END || type == MarkerType::PAUSE_WAIT_FOR_JOB || type == MarkerType::PAUSE_WAIT_FOR_QUEUE_SPACE || type == MarkerType::END_IDLE); }
			bool IsIdleMark() const { return (type == MarkerType::BEGIN_IDLE || type == MarkerType::END_IDLE); }
			bool IsFunctionMark() const { return (type == MarkerType::BEGIN_FUNCTION || type == MarkerType::END_FUNCTION); }

		} profilerData[MaxNumThreads][ProfilerMarkerBufferSize];

		int profilerNextWriteIndex[MaxNumThreads] = {}; // TODO do this atomic?
		int profilerNextReadIndex[MaxNumThreads] = {};
		bool recordNewFrame = true;
		float millisecondLength = 400.0f;
	};
}