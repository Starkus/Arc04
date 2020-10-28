#include <windows.h>
#include <strsafe.h>
#include <stdio.h>

#include "tinyxml/tinyxml2.cpp"
using namespace tinyxml2;

HANDLE g_hStdout;
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

#include "General.h"
#include "Maths.h"
#include "BakeryInterop.h"
#include "Platform.h"
#include "Containers.h"
#include "Geometry.h"
#include "Bakery.h"

GameMemory *g_gameMemory;

#include "Memory.cpp"

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

int ProcessMetaFile(const char *filename, const char *fullDataDir)
{
	XMLError xmlError;

	tinyxml2::XMLDocument doc;
	xmlError = doc.LoadFile(filename);
	if (xmlError != XML_SUCCESS)
	{
		Log("ERROR! Parsing XML file \"%s\" (%s)\n", filename, doc.ErrorStr());
		return xmlError;
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
	};

	return 0;
}

int main(int argc, char **argv)
{
	(void) argc, argv;

	AttachConsole(ATTACH_PARENT_PROCESS);
	g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	GameMemory gameMemory;
	g_gameMemory = &gameMemory;

	gameMemory.stackMem = malloc(stackSize);
	gameMemory.stackPtr = gameMemory.stackMem;
	gameMemory.frameMem = malloc(frameSize);
	gameMemory.framePtr = gameMemory.frameMem;
	gameMemory.transientMem = malloc(transientSize);
	gameMemory.transientPtr = gameMemory.transientMem;

	Array_FileCacheEntry cache;
	ArrayInit_FileCacheEntry(&cache, 1024, TransientAlloc);

	int error = 0;

	char fullDataDir[MAX_PATH];
	GetDataPath(fullDataDir);

	char metaWildcard[MAX_PATH];
	sprintf(metaWildcard, "%s*.meta", fullDataDir);
	WIN32_FIND_DATA findData;
	char fullName[MAX_PATH];
	while (1)
	{
		HANDLE searchHandle = FindFirstFileA(metaWildcard, &findData);
		while (1)
		{
			sprintf(fullName, "%s%s", fullDataDir, findData.cFileName);

			bool changed = Win32DidFileChange(fullName, cache);

			if (!changed)
			{
				changed = DidMetaDependenciesChange(fullName, fullDataDir, cache);
			}

			if (changed)
			{
				Log("-- Detected file %s ---------------------\n", findData.cFileName);
				error = ProcessMetaFile(fullName, fullDataDir);

				//Log("Used %.02fkb of frame allocator\n", ((u8 *)framePtr - (u8 *)frameMem) / 1024.0f);
				FrameWipe();

				if (error != 0)
				{
					Log("Failed to build with error code %x\n", error);
				}
				Log("\n");
			}

			if (!FindNextFileA(searchHandle, &findData))
				break;
		}

		// Process dirty cache entries
		for (u32 i = 0; i < cache.size; ++i)
		{
			FileCacheEntry *cacheEntry = &cache[i];
			cacheEntry->changed = false;
		}

#if 0
		Sleep(500);
#else
		break;
#endif
	}

	free(gameMemory.stackMem);
	free(gameMemory.frameMem);

	return error;
}

#undef FilePosition
#undef FileAlign
