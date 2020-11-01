struct Win32FileTime
{
	FILETIME lastWriteTime;
};
static_assert(sizeof(Win32FileTime) <= sizeof(PlatformFileTime),
		"Win32FileTime doesn't fit in opaque handle!");

struct Win32FindData
{
	WIN32_FIND_DATA findData;
};
static_assert(sizeof(Win32FindData) <= sizeof(PlatformFindData),
		"Win32FindData doesn't fit in opaque handle!");

struct Win32SearchHandle
{
	HANDLE handle;
};
static_assert(sizeof(Win32SearchHandle) <= sizeof(PlatformSearchHandle),
		"Win32SearchHandle doesn't fit in opaque handle!");

typedef HANDLE FileHandle;

enum
{
	SEEK_SET = FILE_BEGIN,
	SEEK_CUR = FILE_CURRENT,
	SEEK_END = FILE_END
};

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

	if (file == INVALID_HANDLE_VALUE)
	{
		*fileBuffer = nullptr;
	}
	else
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

bool PlatformGetLastWriteTime(const char *filename, PlatformFileTime *writeTime)
{
	Win32FileTime *wWriteTime = (Win32FileTime *)writeTime;
	wWriteTime->lastWriteTime = Win32GetLastWriteTime(filename);
	return true;
}

int PlatformCompareFileTime(PlatformFileTime *a, PlatformFileTime *b)
{
	Win32FileTime *wa = (LinuxFileTime *)a;
	Win32FileTime *wb = (LinuxFileTime *)b;
	return CompareFileTime(wa->lastWriteTime, wb->lastWriteTime);
}

bool PlatformFileExists(const char *filename)
{
	return Win32FileExists(filename);
}

FileHandle PlatformOpenForWrite(const char *filename)
{
	HANDLE file = CreateFileA(
			filename,
			GENERIC_WRITE,
			0, // Share
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
			);
	ASSERT(file != INVALID_HANDLE_VALUE);

	return file;
}

u64 PlatformWriteToFile(FileHandle file, void *buffer, u64 size)
{
	DWORD writtenBytes;
	WriteFile(
			file,
			buffer,
			(DWORD)size,
			&writtenBytes,
			nullptr
			);
	ASSERT(writtenBytes == size);

	return (u64)writtenBytes;
}

u64 PlatformFileSeek(FileHandle file, i64 shift, int mode)
{
	LARGE_INTEGER lInt;
	LARGE_INTEGER lIntRes;
	lInt.QuadPart = shift;

	SetFilePointerEx(file, lInt, &lIntRes, mode);

	return lIntRes.QuadPart;
}

PlatformSearchHandle PlatformFindFirstFile(const char *filter, PlatformFindData *findData)
{
	char filter[MAX_PATH];
	sprintf(filter, "%s*", folder);

	PlatformSearchHandle result;
	Win32SearchHandle *win32Handle = (Win32SearchHandle *)result;
	Win32FindData *win32FindData = (Win32FindData *)findData;
	*win32Handle = FindFirstFileA(filter, &win32FindData->findData);
	return result;
}
