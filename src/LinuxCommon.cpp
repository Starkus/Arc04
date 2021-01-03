struct LinuxFileTime
{
	time_t lastWriteTime;
};
static_assert(sizeof(LinuxFileTime) <= sizeof(PlatformFileTime),
		"LinuxFileTime doesn't fit in opaque handle!");

struct LinuxSearchHandle
{
	DIR *dirPtr;
};
static_assert(sizeof(LinuxSearchHandle) <= sizeof(PlatformSearchHandle),
		"LinuxSearchHandle doesn't fit in opaque handle!");

struct LinuxFindData
{
	dirent *findData;
};
static_assert(sizeof(LinuxFindData) <= sizeof(PlatformFindData),
		"LinuxFindData doesn't fit in opaque handle!");

struct LinuxFileHandle
{
	int handle;
};
static_assert(sizeof(LinuxFileHandle) <= sizeof(FileHandle),
		"LinuxFileHandle doesn't fit in opaque handle!");

#define Sleep(...) sleep(__VA_ARGS__)

void Log(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	vprintf(format, args);

	va_end(args);
}

int LinuxReadEntireFile(const char *filename, u8 **fileBuffer, u64 *fileSize, void *(*allocFunc)(u64))
{
	int file = open(filename, O_RDONLY);
	if (file == -1)
	{
		Log("Error %s opening file! %s\n", strerror(errno), filename);
		return 1;
	}

	struct stat fileStat;
	fstat(file, &fileStat);
	*fileSize = fileStat.st_size;

	*fileBuffer = (u8 *)allocFunc(*fileSize);

	u64 bytesRead = read(file, *fileBuffer, *fileSize);
	if (bytesRead == 0)
	{
		Log("Error %s reading from file! %s\n", strerror(errno), filename);
		close(file);
		return 2;
	}

	close(file);
	return 0;
}

bool PlatformReadEntireFile(const char *filename, u8 **fileBuffer, u64 *fileSize,
		void *(*allocFunc)(u64))
{
	int error = LinuxReadEntireFile(filename, fileBuffer, fileSize, allocFunc);
	ASSERT(error == 0);

	return error == 0;
}

bool PlatformGetLastWriteTime(const char *filename, PlatformFileTime *writeTime)
{
	struct stat fileStat;
	int statRes = stat(filename, &fileStat);
	if (statRes != 0)
		return false;

	LinuxFileTime *lWriteTime = (LinuxFileTime *)writeTime;
	lWriteTime->lastWriteTime = fileStat.st_mtime;

	return true;
}

int PlatformCompareFileTime(PlatformFileTime *a, PlatformFileTime *b)
{
	LinuxFileTime *la = (LinuxFileTime *)a;
	LinuxFileTime *lb = (LinuxFileTime *)b;
	return difftime(la->lastWriteTime, lb->lastWriteTime);
}

bool PlatformFileExists(const char *filename)
{
	return access(filename, F_OK) == 0;
}

FileHandle PlatformOpenForWrite(const char *filename)
{
	FileHandle result;
	LinuxFileHandle *linuxHandle = (LinuxFileHandle *)&result;

	int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	if (file == -1)
	{
		Log("ERROR! Opening file \"%s\"\n", strerror(errno));
		ASSERT(false);
	}

	linuxHandle->handle = file;
	return result;
}

void PlatformCloseFile(int file)
{
	LinuxFileHandle *linuxHandle = (LinuxFileHandle *)&file;
	close(linuxHandle->handle);
}

u64 PlatformWriteToFile(FileHandle file, const void *buffer, u64 size)
{
	LinuxFileHandle *linuxHandle = (LinuxFileHandle *)&file;

	u64 writtenBytes = write(linuxHandle->handle, buffer, size);
	ASSERT(writtenBytes == size);
	return writtenBytes;
}

u64 PlatformFileSeek(FileHandle file, i64 shift, int mode)
{
	LinuxFileHandle *linuxHandle = (LinuxFileHandle *)&file;

	u64 result = lseek(linuxHandle->handle, shift, mode);
	return result;
}

bool PlatformFindFirstFile(PlatformSearchHandle *searchHandle, const char *folder, PlatformFindData *findData)
{
	PlatformSearchHandle result;
	LinuxSearchHandle *linuxHandle = (LinuxSearchHandle *)&result;
	LinuxFindData *linuxFindData = (LinuxFindData *)findData;

	linuxHandle->dirPtr = opendir(folder);
	linuxFindData->findData = readdir(linuxHandle->dirPtr);

	*searchHandle = result;
	return linuxFindData->findData != nullptr;
}

bool PlatformFindNextFile(PlatformSearchHandle searchHandle, PlatformFindData *findData)
{
	LinuxSearchHandle *linuxHandle = (LinuxSearchHandle *)&searchHandle;
	LinuxFindData *linuxFindData = (LinuxFindData *)findData;
	linuxFindData->findData = readdir(linuxHandle->dirPtr);
	return linuxFindData->findData != nullptr;
}

void PlatformFindClose(PlatformSearchHandle searchHandle)
{
	LinuxSearchHandle *linuxHandle = (LinuxSearchHandle *)&searchHandle;
	closedir(linuxHandle->dirPtr);
}

const char *PlatformGetCurrentFilename(PlatformFindData *findData)
{
	LinuxFindData *linuxFindData = (LinuxFindData *)findData;
	return linuxFindData->findData->d_name;
}

bool PlatformIsDirectory(const char *filename)
{
	struct stat fileStat;
	stat(filename, &fileStat);
	return S_ISDIR(fileStat.st_mode);
}
