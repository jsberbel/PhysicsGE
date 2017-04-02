
struct Renderer
{
	static constexpr int BuffersSize = 0x10000;
	
	
	GLuint vertexArrays[VERTEX_ARRAY_COUNT];
	GLuint vaos[VAO_COUNT];
	GLuint uniforms[UNIFORM_COUNT];
	GLuint programs[PROGRAM_COUNT];
	GLuint textures[TEXTURE_COUNT];
}

{
	glGenBuffers(UNIFORM_COUNT, renderer.uniforms);
	
	glBindBuffer(GL_UNIFORM_BUFFER, renderer.uniforms[DearImgui]);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(Vect2f), nullptr, GL_DYNAMIC_DRAW);
}

{
	glGenTextures(TEXTURE_COUNT, renderer.textures);
	
	// TODO reload imgui
	uint8_t* pixels;
	int width, height;
	ImGui::GetTexDataAsRGBA32(ImGui::GetIO(), &pixels, &width, &height);
	
	glBindTexture(GL_TEXTURE_2D, renderer.textures[DearImgui]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GLenum formats[4] = { GL_RED, GL_RG, GL_RGB, GL_RGBA };

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

{
	glGenBuffers(VERTEX_ARRAY_COUNT, renderer.vertexArrays);
	glGenVertexArrays(VAO_COUNT, renderer.vaos);
	
	
	glBindBuffer(GL_ARRAY_BUFFER, renderer.vertexArrays[DearImgui]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ImDrawVert) * Renderer::BuffersSize, nullptr, GL_DYNAMIC_DRAW);

	glBindVertexArray(renderer.vaos[DearImgui]);

	glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, pos));
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, uv));
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(ImDrawVert), (GLvoid*)offsetof(ImDrawVert, col));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
}

{
	ImGuiIO& io = ImGui::GetIO();
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


inline bool HandleMouse(const MSG& msg, InputData &data_)
{
	if (msg.message == WM_LBUTTONDOWN)
	{
		data_.clicking = true;
		data_.clickDown = true;
		return true;
	}
	else if (msg.message == WM_LBUTTONUP)
	{
		data_.clicking = false;
		return true;
	}
	else if(msg.message == WM_RBUTTONDOWN)
	{
		data_.clickingRight = true;
		data_.clickDownRight = true;
		return true;
	}
	else if (msg.message == WM_RBUTTONUP)
	{
		data_.clickingRight = false;
		return true;
	}
	else if (msg.message == WM_MOUSEMOVE)
	{
		data_.mousePosition.x = GET_X_LPARAM(msg.lParam);
		data_.mousePosition.y = GET_Y_LPARAM(msg.lParam);
		return true;
	}
	else
	{
		return false;
	}
}


{
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		bool fHandled = false;
		if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
		{
			fHandled = HandleMouse(msg, l_Input);
		}
	}
}


{
	
	io.DeltaTime = (float)l_TicksPerFrame / (float)l_PerfCountFrequency;
	
	io.DisplaySize = ImVec2((float)l_Renderer.windowWidth, (float)l_Renderer.windowHeight);
	io.MouseDown[0] = l_Input.clicking;
	io.MouseDown[1] = l_Input.clickingRight;
	ImGui::NewFrame();
	
	
	
	
	// ---
	RenderDearImgui(renderer);
}

inline void RenderDearImgui(const Renderer &renderer)
{
	ImDrawData* draw_data = ImGui::GetDrawData(); // instructions list to draw

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	ImGuiIO& io = ImGui::GetIO();
	int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	if (fb_width == 0 || fb_height == 0)
		return;
	ImGui::ScaleClipRects(draw_data, io.DisplayFramebufferScale);

	// We are using the OpenGL fixed pipeline to make the example code simpler to read!
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST); // draw only a portion of the screen

	GLuint program = renderer.programs[DearImgui];
	glUseProgram(program);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, renderer.uniforms[DearImgui]);
	//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context


	// Setup viewport, orthographic projection matrix
	glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);

	glBindBuffer(GL_UNIFORM_BUFFER, renderer.uniforms[DearImgui]);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Vect2f), (GLvoid*)&io.DisplaySize);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);


	auto texture = renderer.textures[DearImgui];


	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(renderer.vaos[DearImgui]);
	glBindBuffer(GL_ARRAY_BUFFER, renderer.vertexArrays[DearImgui]);

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
				{
					glBindTexture(GL_TEXTURE_2D, *texture);
				}
				else
				{
					glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
				}
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