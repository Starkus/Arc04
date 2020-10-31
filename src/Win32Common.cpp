void Log(const char *format, ...)
{
	char buffer[2048];
	va_list args;
	va_start(args, format);

	StringCbVPrintfA(buffer, sizeof(buffer), format, args);
	OutputDebugStringA(buffer);

	DWORD bytesWritten;
	WriteFile(g_hStdout, buffer, (DWORD)strlen(buffer), &bytesWritten, nullptr);

	va_end(args);
}

inline bool Win32FileExists(const char *filename)
{
	DWORD attrib = GetFileAttributes(filename);
	return attrib != INVALID_FILE_ATTRIBUTES && attrib != FILE_ATTRIBUTE_DIRECTORY;
}

inline bool Win32GetLastWriteTime(const char *filename, FILETIME *lastWriteTime)
{
	WIN32_FILE_ATTRIBUTE_DATA data;
	const bool success = GetFileAttributesEx(filename, GetFileExInfoStandard, &data);
	if (success)
	{
		*lastWriteTime = data.ftLastWriteTime;
	}

	return success;
}

inline FILETIME Win32GetLastWriteTime(const char *filename)
{
	FILETIME lastWriteTime = {};

	WIN32_FILE_ATTRIBUTE_DATA data;
	if (GetFileAttributesEx(filename, GetFileExInfoStandard, &data))
	{
		lastWriteTime = data.ftLastWriteTime;
	}

	return lastWriteTime;
}

DWORD Win32ReadEntireFile(const char *filename, u8 **fileBuffer, void *(*allocFunc)(u64))
{
	HANDLE file = CreateFileA(
			filename,
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
			);
	DWORD error = GetLastError();
	//ASSERT(file != INVALID_HANDLE_VALUE);

	if (file != INVALID_HANDLE_VALUE)
	{
		DWORD fileSize = GetFileSize(file, nullptr);
		ASSERT(fileSize);
		error = GetLastError();

		*fileBuffer = (u8 *)allocFunc(fileSize);
		DWORD bytesRead;
		bool success = ReadFile(
				file,
				*fileBuffer,
				fileSize,
				&bytesRead,
				nullptr
				);
		ASSERT(success);
		ASSERT(bytesRead == fileSize);

		CloseHandle(file);
	}

	return error;
}

DWORD Win32ReadEntireFileText(const char *filename, char **fileBuffer, void *(*allocFunc)(u64))
{
	HANDLE file = CreateFileA(
			filename,
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
			);
	DWORD error = GetLastError();

	if (file != INVALID_HANDLE_VALUE)
	{
		DWORD fileSize = GetFileSize(file, nullptr);
		ASSERT(fileSize);
		error = GetLastError();

		*fileBuffer = (char *)allocFunc(fileSize + 1);
		(*fileBuffer)[fileSize] = 0; // Null terminate!
		DWORD bytesRead;
		bool success = ReadFile(
				file,
				*fileBuffer,
				fileSize,
				&bytesRead,
				nullptr
				);
		ASSERT(success);
		ASSERT(bytesRead == fileSize);

		CloseHandle(file);
	}

	return error;
}
