#include "Profiler.hh"
#include <atomic>
#include "imgui/imgui.h"
#include "imgui/imconfig.h"
#include <string>

namespace Utilities
{
	// Cridar aquesta funció cada cop que volguem registrar una marca al profiler.
	// emparellar cada BEGIN_* amb un END_* i cada PAUSE_* amb un RESUME_*. Els BEGIN_FUNCTION/END_FUNCTION han d'anar aniuats i dins de begin/end
	inline void Profiler::AddProfileMark(MarkerType reason, const void* identifier, const char* functionName, int threadId, int systemID)
	{
		if (recordNewFrame)
		{
			ProfileMarker mark { std::chrono::high_resolution_clock::now(), identifier, functionName, systemID, reason };

			profilerData[threadId][profilerNextWriteIndex[threadId]] = mark;
			profilerNextWriteIndex[threadId] = (profilerNextWriteIndex[threadId] + 1) % ProfilerMarkerBufferSize;
		}
	}

	// crea un "BEGIN_FUNCTION" i quan l'objecte creat es destrueix, un "END_FUNCTION"
	Profiler::MarkGuard Profiler::CreateProfileMarkGuard(const char* functionName, int threadId, int systemID)
	{
		static std::atomic<uintptr_t> identifierSequence = 0;
		void* id = reinterpret_cast<void*>(++identifierSequence);

		AddProfileMark(MarkerType::BEGIN_FUNCTION, id, functionName, threadId, systemID);
		return MarkGuard(this, threadId, id);
	}

	// dibuixa una finestra ImGUI amb la info del darrer frame
	void Profiler::DrawProfilerToImGUI(int numThreads)
	{
		if (ImGui::Begin("Profiler"))
		{
			ImGui::Checkbox("Record", &recordNewFrame);
			ImGui::SliderFloat("Scale", &millisecondLength, 20, 10000, "%.3f", 5);

			ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

			ImDrawList* draw_list = ImGui::GetWindowDrawList();

			std::chrono::high_resolution_clock::time_point earliestTimePoint;
			std::chrono::high_resolution_clock::time_point latestTimePoint;
			bool earliestInitialized = false;
			bool latestInitialized = false;
			for (int l = 0; l < numThreads; l++)
			{

				if (profilerNextReadIndex[l] != profilerNextWriteIndex[l])
				{
					int index = (profilerNextWriteIndex[l] - 1 + ProfilerMarkerBufferSize) % ProfilerMarkerBufferSize;
					const ProfileMarker &marker = profilerData[l][index];

					if (latestTimePoint < marker.timePoint || !latestInitialized)
					{
						latestTimePoint = marker.timePoint;
						latestInitialized = true;
					}
				}
				for (int i = profilerNextReadIndex[l]; i != profilerNextWriteIndex[l]; i = (i + 1) % ProfilerMarkerBufferSize)
				{
					const ProfileMarker &marker = profilerData[l][i];
					if (marker.IsBeginMark() && !marker.IsIdleMark())
					{
						if (earliestTimePoint > marker.timePoint || !earliestInitialized)
						{
							earliestTimePoint = marker.timePoint;
							earliestInitialized = true;
						}
						break;
					}
				}
			}

			auto maxMs = std::chrono::duration_cast<std::chrono::milliseconds>(latestTimePoint - earliestTimePoint).count() + 2;

			if (maxMs > 1000)
			{
				maxMs = 1000;
			}

			float offset = 50;
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			for (int n = 0; n < maxMs; n++)
			{
				if (n > 0) ImGui::SameLine();
				cursor.x = offset + n * millisecondLength;
				float hue = (n < 16) ? 0.33f - n * 0.08f / 15.0f : (n < 33) ? 0.16f - (n - 16) * 0.08f / 15.0f : 0;
				draw_list->AddLine(cursor, ImVec2(cursor.x, cursor.y + 400), ImColor::HSV(hue, 1, 1), 1.0f);
				ImGui::SetCursorPosX(cursor.x);
				ImGui::LabelText("", "%dms", n);
			}

			{
				auto fmaxMs = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(latestTimePoint - earliestTimePoint).count();
				float hue = (fmaxMs < 16) ? 0.33f - fmaxMs * 0.08f / 15.0f : (fmaxMs < 33) ? 0.16f - (fmaxMs - 16) * 0.08f / 15.0f : 0;

				cursor.x = offset + fmaxMs * millisecondLength;
				draw_list->AddLine(cursor, ImVec2(cursor.x, cursor.y + 400), ImColor::HSV(hue, 1, 1), 1.0f);
			}

			auto DrawPeriod = [this, earliestTimePoint, offset](int index, const ProfileMarker &dataMarker, const ProfileMarker &beginMarker, const ProfileMarker &endMarker, std::chrono::high_resolution_clock::duration excTime = std::chrono::high_resolution_clock::duration())
			{
				std::chrono::high_resolution_clock::duration fromFrameStart = beginMarker.timePoint - earliestTimePoint;
				std::chrono::high_resolution_clock::duration fromFrameEnd = endMarker.timePoint - earliestTimePoint;
				std::chrono::high_resolution_clock::duration markDuration = endMarker.timePoint - beginMarker.timePoint;
				std::chrono::high_resolution_clock::duration inclusiveDuration = endMarker.timePoint - dataMarker.timePoint;
				// show

				auto fromFrameStartMs = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(fromFrameStart);
				auto fromFrameEndMs = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(fromFrameEnd);
				auto markDurationMs = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(markDuration);
				auto exclusiveDurationMs = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(excTime);
				auto inclusiveDurationMs = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(inclusiveDuration);

				float beginPosition = fromFrameStartMs.count() > 0 ? fromFrameStartMs.count() * millisecondLength : 0;
				float length = (fromFrameStartMs.count() > 0 ? markDurationMs.count() : fromFrameEndMs.count()) * millisecondLength;

				// cap things at 1 second
				if (beginPosition > 1000 * millisecondLength)
					beginPosition = 1000 * millisecondLength;
				if (length > 1000 * millisecondLength)
					length = 1000 * millisecondLength;
				// cap things at 1 second

				beginPosition += offset;

				if (length < 5.0f)
					length = 5.0f;

				ImGui::SameLine();

				ImGui::PushID(index);
				float hue = dataMarker.IsIdleMark() ? 0 : ((reinterpret_cast<uintptr_t>(dataMarker.identifier) * 19) % 97) / 97.f; //((beginIndex - profilerNextReadIndex[l] + ProfilerMarkerBufferSize) % ProfilerMarkerBufferSize) * 0.05f; // TODO from job type
				ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(hue, 0.6f, 0.6f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(hue, 0.7f, 0.7f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImColor::HSV(hue, 0.8f, 0.8f));

				ImGui::SetCursorPosX(beginPosition);
				ImGui::Button(dataMarker.jobName, ImVec2(length, 0.0f));
				if (ImGui::IsItemHovered())
				{
					const char* additionalInfo =
						(endMarker.type == MarkerType::PAUSE_WAIT_FOR_JOB) ? "\nPAUSED waiting dependencies"
						: (endMarker.type == MarkerType::PAUSE_WAIT_FOR_QUEUE_SPACE) ? "\nPAUSED adding jobs"
						: "";

					std::string fullTime = exclusiveDurationMs.count() <= 0 ? "" :
						std::string("\nExclusive Time: ") + std::to_string(exclusiveDurationMs.count()) + "ms"
						+ "\nInclusive Time: " + std::to_string(inclusiveDurationMs.count()) + "ms";

					ImGui::SetTooltip("%s\nbegins: %fms\nends: %fms\nduration: %fms%s%s", dataMarker.jobName, fromFrameStartMs.count(), fromFrameEndMs.count(), markDurationMs.count(), fullTime.c_str(), additionalInfo);
				}
				ImGui::PopStyleColor(3);

				ImGui::PopID();
			};

			for (int l = 0; l < numThreads; l++)
			{
				ImGui::PushID(l);

				ImGui::LabelText("", "core %d", l);

				for (int beginIndex = profilerNextReadIndex[l]; beginIndex != profilerNextWriteIndex[l]; beginIndex = (beginIndex + 1) % ProfilerMarkerBufferSize)
				{
					const ProfileMarker &beginMarker = profilerData[l][beginIndex];
					if (beginMarker.IsBeginMark())
					{
						for (int endIndex = beginIndex + 1; endIndex != profilerNextWriteIndex[l]; endIndex = (endIndex + 1) % ProfilerMarkerBufferSize)
						{
							const ProfileMarker &endMarker = profilerData[l][endIndex];
							if (beginMarker.identifier == endMarker.identifier && endMarker.IsEndMark())
							{
								DrawPeriod(beginIndex, beginMarker, beginMarker, endMarker);

								beginIndex = endIndex;
								break;
							}
						}
					}
				}

				int depth = 0;
				bool functionFound;
				do
				{
					functionFound = false;

					for (int beginIndex = profilerNextReadIndex[l]; beginIndex != profilerNextWriteIndex[l]; beginIndex = (beginIndex + 1) % ProfilerMarkerBufferSize)
					{
						const ProfileMarker &beginMarker = profilerData[l][beginIndex];
						if (beginMarker.type == MarkerType::BEGIN_FUNCTION)
						{
							int currentDepth = 0;
							bool inRelevantFiber = true;
							const ProfileMarker *lastInterruption = nullptr;
							for (
								int backIndex = (beginIndex == 0) ? ProfilerMarkerBufferSize : (beginIndex - 1);
								((backIndex == 0) ? ProfilerMarkerBufferSize : (backIndex - 1)) != profilerNextReadIndex[l];
								backIndex = (backIndex == 0) ? ProfilerMarkerBufferSize : (backIndex - 1)
								)
							{
								const ProfileMarker &backMarker = profilerData[l][backIndex];
								if (inRelevantFiber)
								{
									if (backMarker.type == MarkerType::BEGIN_FUNCTION)
									{
										++currentDepth;
									}
									else if (backMarker.type == MarkerType::END_FUNCTION)
									{
										--currentDepth;
									}
									else if (backMarker.type == MarkerType::BEGIN)
									{
										//assert(currentDepth >= 0);
										break;
									}
									else if (backMarker.IsBeginMark())
									{
										inRelevantFiber = false;
										lastInterruption = &backMarker;
									}
								}
								else if (backMarker.IsEndMark() && backMarker.identifier == lastInterruption->identifier)
								{
									inRelevantFiber = true;
									lastInterruption = nullptr;
								}
							}

							if (currentDepth == depth)
							{
								long long exclusiveTime = 0;

								if (!functionFound)
								{
									ImGui::LabelText("", "stack %d", depth);
									functionFound = true;
								}
								const ProfileMarker *lastInterruptionBegin = nullptr, *lastInterruptionEnd = nullptr, *firstInterruptionBegin = nullptr;
								for (int endIndex = (beginIndex + 1) % ProfilerMarkerBufferSize; endIndex != profilerNextWriteIndex[l]; endIndex = (endIndex + 1) % ProfilerMarkerBufferSize)
								{
									const ProfileMarker &endMarker = profilerData[l][endIndex];
									int idDisc = ProfilerMarkerBufferSize;
									if (endMarker.type == MarkerType::END_FUNCTION && beginMarker.identifier == endMarker.identifier)
									{
										if (lastInterruptionBegin == nullptr)
										{
											DrawPeriod(beginIndex, beginMarker, beginMarker, endMarker);
										}
										else
										{
											assert(lastInterruptionEnd != nullptr);
											assert(firstInterruptionBegin != nullptr);

											exclusiveTime += (endMarker.timePoint - lastInterruptionEnd->timePoint).count();

											DrawPeriod(beginIndex, beginMarker, beginMarker, *firstInterruptionBegin, std::chrono::high_resolution_clock::duration(exclusiveTime));
											ImGui::PushID(idDisc);
											DrawPeriod(endIndex, beginMarker, *lastInterruptionEnd, endMarker, std::chrono::high_resolution_clock::duration(exclusiveTime));
											ImGui::PopID();
										}
										break;
									}
									else if (endMarker.IsBeginMark() && endMarker.identifier == lastInterruptionBegin->identifier)
									{
										lastInterruptionEnd = &endMarker;
									}
									else if (endMarker.IsEndMark() && (lastInterruptionBegin == nullptr || endMarker.identifier == lastInterruptionBegin->identifier))
									{
										if (lastInterruptionBegin != nullptr)
										{
											exclusiveTime += (endMarker.timePoint - lastInterruptionEnd->timePoint).count();

											assert(firstInterruptionBegin != nullptr);
											ImGui::PushID(idDisc);
											DrawPeriod(endIndex, beginMarker, *lastInterruptionEnd, endMarker, std::chrono::high_resolution_clock::duration(exclusiveTime));
											ImGui::PopID();
											++idDisc;
										}
										else
										{
											assert(exclusiveTime == 0);
											exclusiveTime = (endMarker.timePoint - beginMarker.timePoint).count();

											assert(firstInterruptionBegin == nullptr);
											firstInterruptionBegin = &endMarker;
										}
										lastInterruptionBegin = &endMarker;
									}
								}
							}
						}
					}

					++depth;
				} while (functionFound);


				if (recordNewFrame)
					profilerNextReadIndex[l] = profilerNextWriteIndex[l];

				ImGui::PopID();

				ImGui::LabelText("", "-", l);
			}
			ImGui::EndChild();
		}
		else if (recordNewFrame)
		{

			for (int l = 0; l < numThreads; l++)
			{
				profilerNextReadIndex[l] = profilerNextWriteIndex[l];
			}
		}
		ImGui::End();
	}
}