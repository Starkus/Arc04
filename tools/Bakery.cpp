#include <windows.h>
#include <strsafe.h>
#include <stdio.h>

#include "tinyxml/tinyxml2.cpp"
using namespace tinyxml2;

#include "General.h"
#include "MemoryAlloc.h"
#include "Maths.h"
#include "BakeryInterop.h"
#include "Containers.h"
#include "Render.h"
#include "Geometry.h"
#include "Platform.h"
#include "Bakery.h"

HANDLE g_hStdout;
Memory *g_memory;

#include "Win32Common.cpp"
#include "MemoryAlloc.cpp"

void GetDataPath(char *dataPath)
{
	GetModuleFileNameA(0, dataPath, MAX_PATH);

	// Get parent directory
	char *lastSlash = dataPath;
	char *secondLastSlash = dataPath;
	for (char *scan = dataPath; *scan != 0; ++scan)
	{
		if (*scan == '\\')
		{
			secondLastSlash = lastSlash;
			lastSlash = scan;
		}
	}

	// Go into data dir
	strcpy(secondLastSlash + 1, dataDir);
}

bool Win32DidFileChange(const char *fullName, Array_FileCacheEntry &cache)
{
	bool changed = true;
	FileCacheEntry *cacheEntry = 0;
	FILETIME newWriteTime;
	if (!Win32GetLastWriteTime(fullName, &newWriteTime))
	{
		Log("ERROR! Couldn't retrieve file write time: %s\n", fullName);
		return false;
	}

	for (u32 i = 0; i < cache.size; ++i)
	{
		if (strcmp(fullName, cache[i].filename) == 0)
		{
			cacheEntry = &cache[i];

			if (!cacheEntry->changed && CompareFileTime(&newWriteTime, &cache[i].lastWriteTime) == 0)
			{
				changed = false;
			}
			break;
		}
	}
	if (changed)
	{
		if (!cacheEntry)
		{
			cacheEntry = &cache[cache.size++];
			strcpy(cacheEntry->filename, fullName);
		}
		cacheEntry->lastWriteTime = newWriteTime;
		cacheEntry->changed = true;
	}
	return changed;
}

HANDLE OpenForWrite(const char *filename)
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

u64 WriteToFile(HANDLE file, void *buffer, u64 size)
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

u64 FileSeek(HANDLE file, i64 shift, DWORD mode)
{
	LARGE_INTEGER lInt;
	LARGE_INTEGER lIntRes;
	lInt.QuadPart = shift;

	SetFilePointerEx(file, lInt, &lIntRes, mode);

	return lIntRes.QuadPart;
}

#define FilePosition(file) FileSeek(file, 0, FILE_CURRENT)
#define FileAlign(fileHandle) FileSeek(fileHandle, FilePosition(fileHandle) & 0b11, FILE_CURRENT)

void GetOutputFilename(const char *metaFilename, char *outputFilename)
{
	strcpy(outputFilename, metaFilename);
	// Change extension
	char *lastDot = 0;
	for (char *scan = outputFilename; ; ++scan)
	{
		if (*scan == '.')
		{
			lastDot = scan;
		}
		else if (*scan == 0)
		{
			if (!lastDot)
				lastDot = scan;
			break;
		}
	}
	strcpy(lastDot, ".bin\0");
	Log("Output name: %s\n", outputFilename);
}

#include "BakeryGeometry.cpp"
#include "BakeryCollada.cpp"

bool DidMetaDependenciesChange(const char *metaFilename, const char *fullDataDir, Array_FileCacheEntry &cache)
{
	XMLError xmlError;

	tinyxml2::XMLDocument doc;
	xmlError = doc.LoadFile(metaFilename);
	if (xmlError != XML_SUCCESS)
	{
		// Don't log errors in this function not to spam the output constantly
		//Log("ERROR! Parsing XML file \"%s\" (%s)\n", metaFilename, doc.ErrorStr());
		return false;
	}

	XMLElement *rootEl = doc.FirstChildElement("meta");
	if (!rootEl)
	{
		//Log("ERROR! Meta root node not found\n");
		return false;
	}

	bool somethingChanged = false;
	XMLElement *fileEl = rootEl->FirstChildElement();
	while (fileEl)
	{
		const char *filename = fileEl->FirstChild()->ToText()->Value();
		char fullName[MAX_PATH];
		sprintf(fullName, "%s%s", fullDataDir, filename);

		if (Win32FileExists(fullName) && Win32DidFileChange(fullName, cache))
		{
			somethingChanged = true;
		}

		fileEl = fileEl->NextSiblingElement();
	}
	return somethingChanged;
}

ErrorCode ProcessMetaFile(const char *filename, const char *fullDataDir)
{
	XMLError xmlError;

	tinyxml2::XMLDocument doc;
	xmlError = doc.LoadFile(filename);
	if (xmlError != XML_SUCCESS)
	{
		Log("ERROR! Parsing XML file \"%s\" (%s)\n", filename, doc.ErrorStr());
		return (ErrorCode)xmlError;
	}

	XMLElement *rootEl = doc.FirstChildElement("meta");
	if (!rootEl)
	{
		Log("ERROR! Meta root node not found\n");
		return ERROR_META_NOROOT;
	}

	DynamicArray_RawVertex finalVertices = {};
	Array_u16 finalIndices = {};
	Skeleton skeleton = {};
	DynamicArray_Animation animations = {};

	DynamicArrayInit_Animation(&animations, 16, FrameAlloc);

	const char *typeStr = rootEl->FindAttribute("type")->Value();
	Log("Meta type: %s\n", typeStr);

	MetaType type = METATYPE_INVALID;
	for (int i = 0; i < METATYPE_COUNT; ++i)
	{
		if (strcmp(typeStr, MetaTypeNames[i]) == 0)
		{
			type = (MetaType)i;
		}
	}
	if (type == METATYPE_INVALID)
	{
		Log("ERROR! Invalid meta type\n");
		return ERROR_META_WRONG_TYPE;
	}

	switch(type)
	{
	case METATYPE_MESH:
	case METATYPE_SKINNED_MESH:
	case METATYPE_TRIANGLE_DATA:
	case METATYPE_POINTS:
	{
		ProcessMetaFileCollada(type, rootEl, filename, fullDataDir);
	} break;
	case METATYPE_SHADER:
	{
		char outputName[MAX_PATH];
		GetOutputFilename(filename, outputName);

		char vertexShaderFile[MAX_PATH];
		char fragmentShaderFile[MAX_PATH];
		bool vertexShaderFileFound = false;
		bool fragmentShaderFileFound = false;

		char parentDir[MAX_PATH];
		strcpy(parentDir, filename);
		char *lastSlash = 0;
		for (char *scan = parentDir; *scan; ++scan)
		{
			if (*scan == '\\')
				lastSlash = scan;
		}
		*(lastSlash + 1) = 0;

		XMLElement *shaderEl = rootEl->FirstChildElement("shader");
		while (shaderEl)
		{
			const char *shaderFile = shaderEl->FirstChild()->ToText()->Value();
			const XMLAttribute *typeAttr = shaderEl->FindAttribute("type");
			const char *shaderType = typeAttr->Value();
			if (strcmp(shaderType, "VERTEX") == 0)
			{
				sprintf(vertexShaderFile, "%s%s", parentDir, shaderFile);
				vertexShaderFileFound = true;
			}
			else if (strcmp(shaderType, "FRAGMENT") == 0)
			{
				sprintf(fragmentShaderFile, "%s%s", parentDir, shaderFile);
				fragmentShaderFileFound = true;
			}

			shaderEl = shaderEl->NextSiblingElement("shader");
		}
		if (!vertexShaderFileFound)
		{
			Log("ERROR! Missing vertex shader!\n");
			return ERROR_MISSING_SHADER;
		}
		if (!fragmentShaderFileFound)
		{
			Log("ERROR! Missing fragment shader!\n");
			return ERROR_MISSING_SHADER;
		}

		// Output
		HANDLE file = OpenForWrite(outputName);

		BakeryShaderHeader header;
		u64 vertexShaderBlobOffset = FileSeek(file, sizeof(header), FILE_BEGIN);

		char *fileBuffer;
		Win32ReadEntireFileText(vertexShaderFile, &fileBuffer, FrameAlloc);
		WriteToFile(file, fileBuffer, strlen(fileBuffer) + 1); // Include null termination

		u64 fragmentShaderBlobOffset = FilePosition(file);
		Win32ReadEntireFileText(fragmentShaderFile, &fileBuffer, FrameAlloc);
		WriteToFile(file, fileBuffer, strlen(fileBuffer) + 1);

		FileSeek(file, 0, FILE_BEGIN);
		header.vertexShaderBlobOffset = vertexShaderBlobOffset;
		header.fragmentShaderBlobOffset = fragmentShaderBlobOffset;
		WriteToFile(file, &header, sizeof(header));

		CloseHandle(file);
	};
	};

	return ERROR_OK;
}

ErrorCode FindMetaFilesRecursive(const char *folder, Array_FileCacheEntry &cache)
{
	char filter[MAX_PATH];
	sprintf(filter, "%s*", folder);
	WIN32_FIND_DATA findData;
	char fullName[MAX_PATH];

	ErrorCode error = ERROR_OK;

	HANDLE searchHandle = FindFirstFileA(filter, &findData);
	while (1)
	{
		if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
			goto next;

		sprintf(fullName, "%s%s", folder, findData.cFileName);

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			char newFolder[MAX_PATH];
			sprintf(newFolder, "%s\\", fullName);
			FindMetaFilesRecursive(newFolder, cache);
			goto next;
		}

		// Check if it's a .meta file
		const char *lastDot = 0;
		for (const char *scan = findData.cFileName; *scan; ++scan)
		{
			if (*scan == '.') lastDot = scan;
		}
		if (!lastDot || strcmp(lastDot, ".meta") != 0)
			goto next;

		bool changed = Win32DidFileChange(fullName, cache);
		if (!changed)
		{
			changed = DidMetaDependenciesChange(fullName, folder, cache);
		}

		if (!changed)
			goto next;

		Log("-- Detected file %s ---------------------\n", findData.cFileName);
		error = ProcessMetaFile(fullName, folder);

		//Log("Used %.02fkb of frame allocator\n", ((u8 *)framePtr - (u8 *)frameMem) / 1024.0f);
		FrameWipe();

		if (error != 0)
		{
			Log("Failed to build with error code %x\n", error);
		}
		Log("\n");

next:
		if (!FindNextFileA(searchHandle, &findData))
			break;
	}
	FindClose(searchHandle);

	return error;
}

int main(int argc, char **argv)
{
	(void) argc, argv;

	AttachConsole(ATTACH_PARENT_PROCESS);
	g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	Memory memory;
	g_memory = &memory;

	memory.stackMem = malloc(stackSize);
	memory.stackPtr = memory.stackMem;
	memory.frameMem = malloc(frameSize);
	memory.framePtr = memory.frameMem;
	memory.transientMem = malloc(transientSize);
	memory.transientPtr = memory.transientMem;

	Array_FileCacheEntry cache;
	ArrayInit_FileCacheEntry(&cache, 1024, TransientAlloc);

	int error = 0;

	char fullDataDir[MAX_PATH];
	GetDataPath(fullDataDir);

	while (1)
	{
		FindMetaFilesRecursive(fullDataDir, cache);

		// Process dirty cache entries
		for (u32 i = 0; i < cache.size; ++i)
		{
			FileCacheEntry *cacheEntry = &cache[i];
			cacheEntry->changed = false;
		}

#if DEBUG_BUILD
		break;
#else
		Sleep(500);
#endif
	}

	free(memory.stackMem);
	free(memory.frameMem);

	return error;
}

#undef FilePosition
#undef FileAlign
