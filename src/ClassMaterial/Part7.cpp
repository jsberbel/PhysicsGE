
	bool keyboard[256] = {};

	glm::vec2 position = {};
	
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		bool processed = false;
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

		if(!processed)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	
	
	float horizontal = 0, vertical = 0;
	if (keyboard['A'])
	{
		horizontal += -1;
	}
	if (keyboard['D'])
	{
		horizontal += +1;
	}
	if (keyboard['W'])
	{
		vertical += +1;
	}
	if (keyboard['S'])
	{
		vertical += -1;
	}
	
	
	
	position.x += horizontal * 0.016f;
	position.y += vertical * 0.016f;
	
	
	InstanceData instanceData = { glm::translate(glm::mat4(), glm::vec3(position, 0)), glm::vec4(1,1,1,1) };