#include "General.h"

// Windows
#if TARGET_WINDOWS
#include <windows.h>
#include <strsafe.h>
#else
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#endif

#include "tinyxml/tinyxml2.cpp"
using namespace tinyxml2;

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "MemoryAlloc.h"
#include "Maths.h"
#include "BakeryInterop.h"
#include "Containers.h"
#include "Render.h"
#include "Geometry.h"
#include "Platform.h"
#include "Bakery.h"

Memory *g_memory;

// Windows
#if TARGET_WINDOWS
#include "Win32Common.cpp"
// Linux
#else
#include "LinuxCommon.cpp"
#endif

#define LOG(...) Log(__VA_ARGS__)

#include "MemoryAlloc.cpp"

void GetDataPath(char *dataPath)
{
#if TARGET_WINDOWS
	GetModuleFileNameA(0, dataPath, MAX_PATH);

	// Get parent directory
	char *lastSlash = dataPath;
	char *secondLastSlash = dataPath;
	for (char *scan = dataPath; *scan != 0; ++scan)
	{
		if (*scan == '\\') *scan = '/';
		if (*scan == '/')
		{
			secondLastSlash = lastSlash;
			lastSlash = scan;
		}
	}

	// Go into data dir
	strcpy(secondLastSlash + 1, dataDir);
#else
	// Just use working directory in Unix
	getcwd(dataPath, PATH_MAX);

	// Get parent directory
	char *last = dataPath + strlen(dataPath) - 1;
	if (*last != '/')
	{
		*++last = '/';
	}

	// Go into data dir
	strcpy(last + 1, dataDir);
#endif
}

bool DidFileChange(const char *fullName, Array_FileCacheEntry &cache)
{
	bool changed = true;
	FileCacheEntry *cacheEntry = 0;
	PlatformFileTime newWriteTime;
	if (!PlatformGetLastWriteTime(fullName, &newWriteTime))
	{
		Log("ERROR! Couldn't retrieve file write time: %s\n", fullName);
		return false;
	}

	for (u32 i = 0; i < cache.size; ++i)
	{
		if (strcmp(fullName, cache[i].filename) == 0)
		{
			cacheEntry = &cache[i];

			if (!cacheEntry->changed && PlatformCompareFileTime(&newWriteTime, &cache[i].lastWriteTime) == 0)
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
			cacheEntry = ArrayAdd_FileCacheEntry(&cache);
			strcpy(cacheEntry->filename, fullName);
		}
		cacheEntry->lastWriteTime = newWriteTime;
		cacheEntry->changed = true;
	}
	return changed;
}

#define FilePosition(file) PlatformFileSeek(file, 0, SEEK_CUR)
#define FileAlign(fileHandle) PlatformFileSeek(fileHandle, FilePosition(fileHandle) & 0b11, SEEK_CUR)

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
	strcpy(lastDot, ".b\0");
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

		if (PlatformFileExists(fullName) && DidFileChange(fullName, cache))
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
	case METATYPE_COLLISION_MESH:
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
			if (*scan == '/')
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
		FileHandle file = PlatformOpenForWrite(outputName);

		BakeryShaderHeader header;
		u64 vertexShaderBlobOffset = PlatformFileSeek(file, sizeof(header), SEEK_SET);

		const u8 zero = 0;
		u8 *fileBuffer;
		u64 fileSize;
		PlatformReadEntireFile(vertexShaderFile, &fileBuffer, &fileSize, FrameAlloc);
		PlatformWriteToFile(file, fileBuffer, fileSize);
		PlatformWriteToFile(file, &zero, 1);

		u64 fragmentShaderBlobOffset = FilePosition(file);
		PlatformReadEntireFile(fragmentShaderFile, &fileBuffer, &fileSize, FrameAlloc);
		PlatformWriteToFile(file, fileBuffer, fileSize);
		PlatformWriteToFile(file, &zero, 1);

		PlatformFileSeek(file, 0, SEEK_SET);
		header.vertexShaderBlobOffset = vertexShaderBlobOffset;
		header.fragmentShaderBlobOffset = fragmentShaderBlobOffset;
		PlatformWriteToFile(file, &header, sizeof(header));

		PlatformCloseFile(file);
	} break;
	case METATYPE_IMAGE:
	{
		char outputName[MAX_PATH];
		GetOutputFilename(filename, outputName);

		XMLElement *imageEl = rootEl->FirstChildElement("image");
		const char *imageFile = imageEl->FirstChild()->ToText()->Value();

		char fullname[MAX_PATH];
		sprintf(fullname, "%s%s", fullDataDir, imageFile);

		int width, height, components;
		u8 *imageData = stbi_load(fullname, &width, &height, &components, 0);
		ASSERT(imageData);

		FileHandle file = PlatformOpenForWrite(outputName);

		BakeryImageHeader header;
		header.width = width;
		header.height = height;
		header.components = components;
		header.dataBlobOffset = sizeof(BakeryImageHeader);
		PlatformWriteToFile(file, &header, sizeof(BakeryImageHeader));

		PlatformWriteToFile(file, imageData, width * height * components);

		PlatformCloseFile(file);
		stbi_image_free(imageData);
	} break;
	case METATYPE_MATERIAL:
	{
		char outputName[MAX_PATH];
		GetOutputFilename(filename, outputName);

		RawBakeryMaterial rawMaterial = {};

		XMLElement *shaderEl = rootEl->FirstChildElement("shader");
		rawMaterial.shaderFilename = shaderEl->FirstChild()->ToText()->Value();
		Log("Shader: %s\n", rawMaterial.shaderFilename);

		XMLElement *textureEl = rootEl->FirstChildElement("texture");
		while (textureEl)
		{
			const char *textureFilename = textureEl->FirstChild()->ToText()->Value();
			rawMaterial.textureFilenames[rawMaterial.textureCount++] = textureFilename;
			textureEl = textureEl->NextSiblingElement("texture");
			Log("Texture: %s\n", textureFilename);
		}

		FileHandle file = PlatformOpenForWrite(outputName);

		BakeryMaterialHeader header;
		header.textureCount = rawMaterial.textureCount;
		PlatformFileSeek(file, sizeof(header), SEEK_SET);

		header.shaderNameOffset = FilePosition(file);
		PlatformWriteToFile(file, rawMaterial.shaderFilename, strlen(rawMaterial.shaderFilename) + 1);

		header.textureNamesOffset = FilePosition(file);
		for (u32 texIdx = 0; texIdx < rawMaterial.textureCount; ++texIdx)
		{
			PlatformWriteToFile(file, rawMaterial.textureFilenames[texIdx],
					strlen(rawMaterial.textureFilenames[texIdx]) + 1);
		}

		PlatformFileSeek(file, 0, SEEK_SET);
		PlatformWriteToFile(file, &header, sizeof(header));

		PlatformCloseFile(file);
	} break;
	default:
	{
		return ERROR_META_WRONG_TYPE;
	} break;
	};

	return ERROR_OK;
}

ErrorCode FindMetaFilesRecursive(const char *folder, Array_FileCacheEntry &cache)
{
	char fullName[MAX_PATH];

	ErrorCode error = ERROR_OK;

	PlatformFindData findData;
	PlatformSearchHandle searchHandle;
	bool success = PlatformFindFirstFile(&searchHandle, folder, &findData);
	if (!success)
		// Empty directory I guess
		return error;

	do
	{
		const char *curFilename = PlatformGetCurrentFilename(&findData);
		if (strcmp(curFilename, ".") == 0 || strcmp(curFilename, "..") == 0)
			continue;

		sprintf(fullName, "%s%s", folder, curFilename);

		if (PlatformIsDirectory(fullName))
		{
			char newFolder[MAX_PATH];
			sprintf(newFolder, "%s/", fullName);
			FindMetaFilesRecursive(newFolder, cache);
			continue;
		}

		// Check if it's a .meta file
		const char *lastDot = 0;
		for (const char *scan = curFilename; *scan; ++scan)
		{
			if (*scan == '.') lastDot = scan;
		}
		if (!lastDot || strcmp(lastDot, ".meta") != 0)
			continue;

		bool changed = DidFileChange(fullName, cache);
		// Even if meta file changed, check dependencies to register new write times
		changed = DidMetaDependenciesChange(fullName, folder, cache) || changed;

		if (!changed)
			continue;

		Log("-- Detected file %s ---------------------\n", curFilename);
		error = ProcessMetaFile(fullName, folder);

		//Log("Used %.02fkb of frame allocator\n", ((u8 *)framePtr - (u8 *)frameMem) / 1024.0f);
		FrameWipe();

		if (error != 0)
		{
			Log("Failed to build with error code %x\n", error);
		}
		Log("\n");
	}
	while(PlatformFindNextFile(searchHandle, &findData));
	PlatformFindClose(searchHandle);

	return error;
}

int main(int argc, char **argv)
{
	(void) argc, argv;

#if TARGET_WINDOWS
	AttachConsole(ATTACH_PARENT_PROCESS);
	g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	Memory memory = {};
	g_memory = &memory;

	memory.stackMem = malloc(Memory::stackSize);
	memory.stackPtr = memory.stackMem;
	memory.frameMem = malloc(Memory::frameSize);
	memory.framePtr = memory.frameMem;
	memory.transientMem = malloc(Memory::transientSize);
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

	return error;
}
