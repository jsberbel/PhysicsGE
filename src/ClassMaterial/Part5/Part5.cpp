


struct FullFile
{
	uint8_t* data;
	size_t size;
	uint64_t lastWriteTime;
};

inline FullFile ReadFullFile(const wchar_t* path)
{
	FullFile result;

	HANDLE hFile;
	OVERLAPPED ol = {};

	hFile = CreateFileW(
		path,               // file to open
		GENERIC_READ,          // open for reading
		FILE_SHARE_READ,       // share for reading
		NULL,                  // default security
		OPEN_EXISTING,         // existing file only
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // normal file
		NULL);                 // no attr. template

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD fileSizeHigh;
		result.size = GetFileSize(hFile, &fileSizeHigh);
		FILETIME lastWriteTime;
		GetFileTime(hFile, NULL, NULL, &lastWriteTime);
		ULARGE_INTEGER li;
		li.HighPart = lastWriteTime.dwHighDateTime;
		li.LowPart = lastWriteTime.dwLowDateTime;
		result.lastWriteTime = li.QuadPart;

		result.data = new uint8_t[result.size + 1];

		if (ReadFileEx(hFile, result.data, (DWORD)result.size, &ol, nullptr))
		{
			result.data[result.size] = 0; // set the last byte to 0 to help strings
		}
		else
		{
			result.data = nullptr;
			result.size = 0;
		}
		CloseHandle(hFile);
	}
	else
	{
		result.data = nullptr;
		result.size = 0;
	}
	return result;
}

struct VertexTN
{
	glm::vec3 p, n;
	glm::vec4 c;
	glm::vec2 t;
};

struct InstanceData
{
	glm::mat4 modelMatrix;
	glm::vec4 colorModifier;
};

{
	glGenBuffers(1, &vertexBuffer); // creates a buffer. The buffer id is written to the variable "instanceDataBuffer"
	glBindBuffer(GL_VERTEX_BUFFER, vertexBuffer);
}

{	
	glGenBuffers(1, &instanceDataBuffer); // creates a buffer. The buffer id is written to the variable "instanceDataBuffer"
	glBindBuffer(GL_UNIFORM_BUFFER, instanceDataBuffer); // sets "instanceDataBuffer" as the current buffer
	glBufferData(  // sets the buffer content
		GL_UNIFORM_BUFFER,		// type of buffer
		sizeof(InstanceData),	// size of the buffer content
		nullptr,				// content of the buffer
		GL_DYNAMIC_DRAW);		// usage of the buffer. DYNAMIC -> will change frequently. DRAW -> from CPU to GPU

		// we initialize the buffer without content so we can later call "glBufferSubData". Here we are only reserving the size.
		
	glBindBuffer(GL_UNIFORM_BUFFER, 0); // set no buffer as the current buffer
}

{
	InstanceData instanceData = { glm::mat4(), glm::vec4(1,1,1,1) };

	glBindBuffer(GL_UNIFORM_BUFFER, instanceDataBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, // change the contents of the buffer
					0, // offset from the beginning of the buffer
					sizeof(InstanceData), // size of the changes
					(GLvoid*)&instanceData); // new content


	glBindBufferBase(GL_UNIFORM_BUFFER, 0, instanceDataBuffer); // sets "instanceDataBuffer" to the slot "0"
}