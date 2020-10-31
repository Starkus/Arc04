#include <windows.h>
#include <strsafe.h>
#include <GL/gl.h>
#include <GLES3/gl32.h>
#include <GLES3/glext.h>
#include <GLES3/wglext.h>
#include <GLES3/glcorearb.h>
#include <Xinput.h>

#include "General.h"
#include "OpenGL.h"
#include "MemoryAlloc.h"
#include "Containers.h"
#include "Maths.h"
#include "Render.h"
#include "Geometry.h"
#include "Resource.h"
#include "Platform.h"
#include "PlatformCode.h"

DECLARE_ARRAY(Resource);
DECLARE_ARRAY(FILETIME);

struct ResourceBank
{
	Array_Resource resources;
	Array_FILETIME lastWriteTimes;
};

HANDLE g_hStdout;
Memory *g_memory;
ResourceBank *g_resourceBank;

#include "Win32Common.cpp"
#include "OpenGL.cpp"
#include "OpenGLRender.cpp"
#include "MemoryAlloc.cpp"

#include "Game.h"
#include "BakeryInterop.cpp"

#if DEBUG_BUILD
const char *gameDllName = "game_debug.dll";
const char *tempDllName = ".game_temp.dll";
#else
const char *gameDllName = "game.dll";
#endif

// WGL
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
void LoadWGLProcs()
{
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)GL_GetProcAddress("wglChoosePixelFormatARB");
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GL_GetProcAddress("wglCreateContextAttribsARB");
}
/////////

// XInput functions
// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(XInputGetState_t);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	(void) pState, dwUserIndex;
	return 0;
}
static XInputGetState_t *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// Try to load library
void Win32LoadXInput()
{
	HMODULE xInputLibrary = LoadLibraryA("xinput1_3.dll");
	if (xInputLibrary)
	{
		OutputDebugStringA("XInput library found!\n");
		XInputGetState = (XInputGetState_t *)GetProcAddress(xInputLibrary, "XInputGetState");
	}
	else
	{
		OutputDebugStringA("WARNING: XInput library not found!\n");
	}
}

bool ProcessKeyboard(Controller *controller)
{
	MSG message;
	while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
	{
		switch (message.message)
		{
			case WM_QUIT:
			{
				return true;
			} break;
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				const bool isDown = (message.lParam & (1 << 31)) == 0;
				const bool wasDown = (message.lParam & (1 << 30)) != 0;

				auto checkButton = [&isDown, &wasDown](Button &button)
				{
					if (wasDown != isDown)
					{
						button.endedDown = isDown;
						button.changed = true;
					}
				};

				Controller *c = controller;
				switch (message.wParam)
				{
					case 'Q':
						return true;
					case 'W':
						checkButton(c->up);
						break;
					case 'A':
						checkButton(c->left);
						break;
					case 'S':
						checkButton(c->down);
						break;
					case 'D':
						checkButton(c->right);
						break;

					case VK_SPACE:
						checkButton(c->jump);
						break;

					case VK_UP:
						checkButton(c->camUp);
						break;
					case VK_DOWN:
						checkButton(c->camDown);
						break;
					case VK_LEFT:
						checkButton(c->camLeft);
						break;
					case VK_RIGHT:
						checkButton(c->camRight);
						break;

#if DEBUG_BUILD
					case 'H':
						checkButton(c->debugUp);
						break;
					case 'J':
						checkButton(c->debugDown);
						break;
					case 'K':
						checkButton(c->debugUp);
						break;
					case 'L':
						checkButton(c->debugDown);
						break;
#endif
				}
			} break;
			default:
			{
				TranslateMessage(&message);
				DispatchMessage(&message);
			} break;
		}
	}
	return false;
}

void ProcessXInput(Controller *oldController, Controller *controller)
{
	auto processButton = [](WORD xInputButtonState, WORD buttonBit, Button *oldButton, Button *button)
	{
		bool isDown = xInputButtonState & buttonBit;
		if (oldButton->endedDown != isDown)
		{
			button->endedDown = isDown;
			button->changed = true;
		}
	};

	WORD controllerIdx = 0;
	XINPUT_STATE xInputState;
	if (XInputGetState(controllerIdx, &xInputState) == ERROR_SUCCESS)
	{
		XINPUT_GAMEPAD *pad = &xInputState.Gamepad;
		processButton(pad->wButtons, XINPUT_GAMEPAD_A, &oldController->jump, &controller->jump);

		const f32 range = 32000.0f;
		controller->leftStick.x = pad->sThumbLX / range;
		controller->leftStick.y = pad->sThumbLY / range;
		controller->rightStick.x = pad->sThumbRX / range;
		controller->rightStick.y = pad->sThumbRY / range;
	}
}
/////////

struct Win32GameCode
{
	HMODULE dll;
	bool isLoaded;
	FILETIME lastWriteTime;

	StartGame_t *StartGame;
	UpdateAndRenderGame_t *UpdateAndRenderGame;
};

void ChangeExtension(char *buffer, const char *newExtension)
{
	char *lastDot = 0;
	for (char *scan = buffer; *scan; ++scan)
		if (*scan == '.')
			lastDot = scan;
	ASSERT(lastDot);
	strcpy(lastDot + 1, newExtension);
}

void Win32LoadGameCode(Win32GameCode *gameCode, const char *dllFilename, const char *tempDllFilename)
{
	Log("Loading game code\n");
#if DEBUG_BUILD
	CopyFile(dllFilename, tempDllFilename, false);
	gameCode->dll = LoadLibraryA(tempDllFilename);
	// Copy pdb
	{
		char pdbName[MAX_PATH];
		strcpy(pdbName, dllFilename);
		ChangeExtension(pdbName, "pdb");
		char tempPdbName[MAX_PATH];
		strcpy(tempPdbName, tempDllFilename);
		ChangeExtension(tempPdbName, "pdb");
		CopyFile(pdbName, tempPdbName, false);
	}
#else
	gameCode->dll = LoadLibraryA(dllFilename);
#endif
	ASSERT(gameCode->dll);

	gameCode->StartGame = (StartGame_t *)GetProcAddress(gameCode->dll, "StartGame");
	ASSERT(gameCode->StartGame);
	gameCode->UpdateAndRenderGame = (UpdateAndRenderGame_t *)GetProcAddress(gameCode->dll, "UpdateAndRenderGame");
	ASSERT(gameCode->UpdateAndRenderGame);

	gameCode->isLoaded = true;
}

void Win32UnloadGameCode(Win32GameCode *gameCode)
{
	if (gameCode->dll)
	{
		Log("Unloading game code\n");
		FreeLibrary(gameCode->dll);
		gameCode->isLoaded = false;
		gameCode->StartGame = StartGameStub;
		gameCode->UpdateAndRenderGame = UpdateAndRenderGameStub;
	}
}

LRESULT CALLBACK Win32WindowCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CLOSE:
	{
		PostQuitMessage(0);
	} break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

PLATFORM_READ_ENTIRE_FILE(ReadEntireFile)
{
	char fullname[MAX_PATH];
	DWORD written = GetCurrentDirectory(MAX_PATH, fullname);
	fullname[written++] = '/';
	strcpy(fullname + written, filename);

	u8 *fileBuffer;
	DWORD error = Win32ReadEntireFile(filename, &fileBuffer, allocFunc);
	ASSERT(error == ERROR_SUCCESS);

	return fileBuffer;
}

Resource *CreateResource(const char *filename)
{
	Resource result = {};

	strcpy(result.filename, filename);

	FILETIME writeTime = Win32GetLastWriteTime(filename);
	g_resourceBank->lastWriteTimes[g_resourceBank->lastWriteTimes.size++] = writeTime;

	Resource *resource = &g_resourceBank->resources[g_resourceBank->resources.size++];
	*resource = result;
	return resource;
}

RESOURCE_LOAD_MESH(ResourceLoadMesh)
{
	Resource *result = CreateResource(filename);
	result->type = RESOURCETYPE_MESH;

	u8 *fileBuffer;
	DWORD error = Win32ReadEntireFile(filename, &fileBuffer, FrameAlloc);
	ASSERT(error == ERROR_SUCCESS);

	Vertex *vertexData;
	u16 *indexData;
	u32 vertexCount;
	u32 indexCount;
	ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount);

	result->mesh.deviceMesh = CreateDeviceIndexedMesh();
	SendIndexedMesh(&result->mesh.deviceMesh, vertexData, vertexCount, indexData,
			indexCount, false);

	return result;
}

RESOURCE_LOAD_SKINNED_MESH(ResourceLoadSkinnedMesh)
{
	Resource *result = CreateResource(filename);
	result->type = RESOURCETYPE_SKINNEDMESH;

	ResourceSkinnedMesh *skinnedMesh = &result->skinnedMesh;

	u8 *fileBuffer;
	DWORD error = Win32ReadEntireFile(filename, &fileBuffer, FrameAlloc);
	ASSERT(error == ERROR_SUCCESS);

	SkinnedVertex *vertexData;
	u16 *indexData;
	u32 vertexCount;
	u32 indexCount;
	ReadSkinnedMesh(fileBuffer, skinnedMesh, &vertexData, &indexData, &vertexCount, &indexCount);

	// @Broken: This allocs things on transient memory and never frees them
	skinnedMesh->deviceMesh = CreateDeviceIndexedSkinnedMesh();
	SendIndexedSkinnedMesh(&skinnedMesh->deviceMesh, vertexData, vertexCount, indexData,
			indexCount, false);

	return result;
}

RESOURCE_LOAD_LEVEL_GEOMETRY_GRID(ResourceLoadLevelGeometryGrid)
{
	Resource *result = CreateResource(filename);
	result->type = RESOURCETYPE_LEVELGEOMETRYGRID;

	ResourceGeometryGrid *geometryGrid = &result->geometryGrid;

	u8 *fileBuffer;
	DWORD error = Win32ReadEntireFile(filename, &fileBuffer, FrameAlloc);
	ASSERT(error == ERROR_SUCCESS);

	ReadTriangleGeometry(fileBuffer, geometryGrid);

	return result;
}

RESOURCE_LOAD_POINTS(ResourceLoadPoints)
{
	Resource *result = CreateResource(filename);
	result->type = RESOURCETYPE_POINTS;

	ResourcePointCloud *pointCloud = &result->points;

	u8 *fileBuffer;
	DWORD error = Win32ReadEntireFile(filename, &fileBuffer, FrameAlloc);
	ASSERT(error == ERROR_SUCCESS);

	ReadPoints(fileBuffer, pointCloud);

	return result;
}

RESOURCE_LOAD_SHADER(ResourceLoadShader)
{
	Resource *result = CreateResource(filename);
	result->type = RESOURCETYPE_SHADER;

	ResourceShader *shader = &result->shader;

	u8 *fileBuffer;
	DWORD error = Win32ReadEntireFile(filename, &fileBuffer, FrameAlloc);
	ASSERT(error == ERROR_SUCCESS);

	const char *vertexSource, *fragmentSource;
	ReadBakeryShader(fileBuffer, &vertexSource, &fragmentSource);

	DeviceProgram programHandle = CreateDeviceProgram();

	DeviceShader vertexShaderHandle = CreateShader(SHADERTYPE_VERTEX);
	LoadShader(&vertexShaderHandle, vertexSource);
	AttachShader(programHandle, vertexShaderHandle);

	DeviceShader fragmentShaderHandle = CreateShader(SHADERTYPE_FRAGMENT);
	LoadShader(&fragmentShaderHandle, fragmentSource);
	AttachShader(programHandle, fragmentShaderHandle);

	LinkDeviceProgram(programHandle);

	shader->programHandle = programHandle;
	return result;
}

GET_RESOURCE(GetResource)
{
	for (u32 i = 0; i < g_resourceBank->resources.size; ++i)
	{
		Resource *it = &g_resourceBank->resources[i];
		if (strcmp(it->filename, filename) == 0)
		{
			return it;
		}
	}
	return nullptr;
}

bool ReloadResource(Resource *resource)
{
	u8 *fileBuffer;
	DWORD error;
	if (resource->type == RESOURCETYPE_SHADER)
		error = Win32ReadEntireFileText(resource->filename, (char **)&fileBuffer, FrameAlloc);
	else
		error = Win32ReadEntireFile(resource->filename, &fileBuffer, FrameAlloc);
	if (error != ERROR_SUCCESS)
		return false;

	switch (resource->type)
	{
	case RESOURCETYPE_MESH:
	{
		Vertex *vertexData;
		u16 *indexData;
		u32 vertexCount;
		u32 indexCount;
		ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount);

		SendIndexedMesh(&resource->mesh.deviceMesh, vertexData, vertexCount, indexData,
				indexCount, false);

		return true;
	} break;
	case RESOURCETYPE_SKINNEDMESH:
	{
		ResourceSkinnedMesh *skinnedMesh = &resource->skinnedMesh;

		SkinnedVertex *vertexData;
		u16 *indexData;
		u32 vertexCount;
		u32 indexCount;
		ReadSkinnedMesh(fileBuffer, skinnedMesh, &vertexData, &indexData, &vertexCount, &indexCount);

		// @Broken: This allocs things on transient memory and never frees them
		SendIndexedSkinnedMesh(&resource->skinnedMesh.deviceMesh, vertexData, vertexCount, indexData,
				indexCount, false);

		return true;
	} break;
	case RESOURCETYPE_LEVELGEOMETRYGRID:
	{
		ResourceGeometryGrid *geometryGrid = &resource->geometryGrid;
		ReadTriangleGeometry(fileBuffer, geometryGrid);
		return true;
	} break;

	case RESOURCETYPE_POINTS:
	{
		ResourcePointCloud *pointCloud = &resource->points;
		ReadPoints(fileBuffer, pointCloud);
		return true;
	} break;
	case RESOURCETYPE_SHADER:
	{
		ResourceShader *shader = &resource->shader;

		const char *vertexSource, *fragmentSource;
		ReadBakeryShader(fileBuffer, &vertexSource, &fragmentSource);

		WipeDeviceProgram(shader->programHandle);

		DeviceShader vertexShaderHandle = CreateShader(SHADERTYPE_VERTEX);
		LoadShader(&vertexShaderHandle, vertexSource);
		AttachShader(shader->programHandle, vertexShaderHandle);

		DeviceShader fragmentShaderHandle = CreateShader(SHADERTYPE_FRAGMENT);
		LoadShader(&fragmentShaderHandle, fragmentSource);
		AttachShader(shader->programHandle, fragmentShaderHandle);

		LinkDeviceProgram(shader->programHandle);

		return true;
	} break;
	};
	return false;
}

void Win32Start(HINSTANCE hInstance)
{
	AttachConsole(ATTACH_PARENT_PROCESS);
	g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	WNDCLASSEX windowClass = {};
	ZeroMemory(&windowClass, sizeof(windowClass));
	windowClass.cbSize = sizeof(windowClass);
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = Win32WindowCallback;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = "window";

	bool success = RegisterClassEx(&windowClass);
	ASSERT(success);

	// Fake window
	HWND fakeWindow = CreateWindow("window", "Fake window", WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			0, 0, 1, 1, nullptr, nullptr, hInstance, nullptr);

	// Fake context
	HDC fakeContext = GetDC(fakeWindow);

	PIXELFORMATDESCRIPTOR pfd = {};
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int choose = ChoosePixelFormat(fakeContext, &pfd);
	ASSERT(choose);

	success = SetPixelFormat(fakeContext, choose, &pfd);
	ASSERT(success);

	HGLRC fakeGlContext = wglCreateContext(fakeContext);
	ASSERT(fakeGlContext);
	success = wglMakeCurrent(fakeContext, fakeGlContext);
	ASSERT(success);

	// Real context
	LoadOpenGLProcs();
	LoadWGLProcs();

	HWND windowHandle = CreateWindowA("window", "3DVania",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, 16, 16, 1440, 810, 0, 0, hInstance, 0);


	HDC deviceContext = GetDC(windowHandle);
	const int pixelAttribs[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		WGL_SAMPLES_ARB, 4,
		0
	};

	int pixelFormatId;
	u32 numFormats;
	success = wglChoosePixelFormatARB(deviceContext, pixelAttribs, nullptr, 1, &pixelFormatId,
			&numFormats);
	ASSERT(success);

	DescribePixelFormat(deviceContext, pixelFormatId, sizeof(pfd), &pfd);
	SetPixelFormat(deviceContext, pixelFormatId, &pfd);

	int contextAttribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	HGLRC glContext = wglCreateContextAttribsARB(deviceContext, 0, contextAttribs);
	ASSERT(glContext);

	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(fakeGlContext);
	ReleaseDC(fakeWindow, fakeContext);
	DestroyWindow(fakeWindow);

	success = wglMakeCurrent(deviceContext, glContext);
	ASSERT(success);

	// Get paths
	char executableFilename[MAX_PATH];
	char binPath[MAX_PATH];
	char gameDllFilename[MAX_PATH];
	char tempDllFilename[MAX_PATH];
	{
		GetModuleFileNameA(0, executableFilename, MAX_PATH);
		strcpy(gameDllFilename, executableFilename);
		char *lastSlash = gameDllFilename;
		for (char *scan = gameDllFilename; *scan != 0; ++scan)
			if (*scan == '\\') lastSlash = scan;
		*lastSlash = 0;
		strcpy(binPath, gameDllFilename);

		*lastSlash = '\\';
#if DEBUG_BUILD
		strcpy(lastSlash + 1, tempDllName);
		strcpy(tempDllFilename, gameDllFilename);
#endif
		strcpy(lastSlash + 1, gameDllName);
	}

	Win32LoadXInput();

	// Load game code
	Win32GameCode gameCode;
	Win32LoadGameCode(&gameCode, gameDllFilename, tempDllFilename);
	gameCode.lastWriteTime = Win32GetLastWriteTime(gameDllFilename);

	Controller controller = {};

	LARGE_INTEGER largeInteger;
	QueryPerformanceCounter(&largeInteger);
	u64 lastPerfCounter = largeInteger.LowPart;

	QueryPerformanceFrequency(&largeInteger);
	u64 perfFrequency = largeInteger.LowPart;

	// Allocate memory
	Memory memory;
	memory.frameMem = VirtualAlloc(0, frameSize, MEM_COMMIT, PAGE_READWRITE);
	memory.stackMem = VirtualAlloc(0, stackSize, MEM_COMMIT, PAGE_READWRITE);
	memory.transientMem = VirtualAlloc(0, transientSize, MEM_COMMIT, PAGE_READWRITE);
	memory.framePtr = memory.frameMem;
	memory.stackPtr = memory.stackMem;
	memory.transientPtr = memory.transientMem;
	g_memory = &memory;

	TransientAlloc(sizeof(GameState));

	ResourceBank resourceBank;
	ArrayInit_Resource(&resourceBank.resources, 256, TransientAlloc);
	ArrayInit_FILETIME(&resourceBank.lastWriteTimes, 256, TransientAlloc);
	g_resourceBank = &resourceBank;

	// Pass functions
	PlatformCode platformCode;
	platformCode.Log = Log;
	platformCode.ReadEntireFile = ReadEntireFile;
	platformCode.SetUpDevice = SetUpDevice;
	platformCode.ClearBuffers = ClearBuffers;
	platformCode.GetUniform = GetUniform;
	platformCode.UseProgram = UseProgram;
	platformCode.UniformMat4 = UniformMat4;
	platformCode.RenderIndexedMesh = RenderIndexedMesh;
	platformCode.RenderMesh = RenderMesh;
	platformCode.RenderLines = RenderLines;
	platformCode.CreateDeviceMesh = CreateDeviceMesh;
	platformCode.CreateDeviceIndexedMesh = CreateDeviceIndexedMesh;
	platformCode.CreateDeviceIndexedSkinnedMesh = CreateDeviceIndexedSkinnedMesh;
	platformCode.SendMesh = SendMesh;
	platformCode.SendIndexedMesh = SendIndexedMesh;
	platformCode.SendIndexedSkinnedMesh = SendIndexedSkinnedMesh;
	platformCode.CreateShader = CreateShader;
	platformCode.LoadShader = LoadShader;
	platformCode.AttachShader = AttachShader;
	platformCode.CreateDeviceProgram = CreateDeviceProgram;
	platformCode.LinkDeviceProgram = LinkDeviceProgram;
	platformCode.SetFillMode = SetFillMode;
	platformCode.ResourceLoadMesh = ResourceLoadMesh;
	platformCode.ResourceLoadSkinnedMesh = ResourceLoadSkinnedMesh;
	platformCode.ResourceLoadLevelGeometryGrid = ResourceLoadLevelGeometryGrid;
	platformCode.ResourceLoadPoints = ResourceLoadPoints;
	platformCode.ResourceLoadShader = ResourceLoadShader;
	platformCode.GetResource = GetResource;

	gameCode.StartGame(&memory, &platformCode);

	bool running = true;
	while (running)
	{
		// Hot reload
		FILETIME newDllWriteTime = Win32GetLastWriteTime(gameDllFilename);
		if (CompareFileTime(&newDllWriteTime, &gameCode.lastWriteTime) != 0)
		{
			Win32UnloadGameCode(&gameCode);
			Win32LoadGameCode(&gameCode, gameDllFilename, tempDllFilename);
			gameCode.lastWriteTime = newDllWriteTime;
		}

		for (u32 i = 0; i < resourceBank.resources.size; ++i)
		{
			const char *filename = resourceBank.resources[i].filename;
			FILETIME newWriteTime = Win32GetLastWriteTime(filename);
			if (CompareFileTime(&newWriteTime, &resourceBank.lastWriteTimes[i]) != 0)
			{
				bool reloaded = ReloadResource(&resourceBank.resources[i]);
				if (reloaded)
				{
					Log("Reloaded resource \"%s\"\n", filename);
					resourceBank.lastWriteTimes[i] = newWriteTime;
				}
			}
		}

		QueryPerformanceCounter(&largeInteger);
		const u64 newPerfCounter = largeInteger.LowPart;
		const f64 time = (f64)newPerfCounter / (f64)perfFrequency;
		const f32 deltaTime = (f32)(newPerfCounter - lastPerfCounter) / (f32)perfFrequency;

		// Check events
		Controller oldController = controller;
		for (int buttonIdx = 0; buttonIdx < ArrayCount(controller.b); ++buttonIdx)
			controller.b[buttonIdx].changed = false;

		bool stop = ProcessKeyboard(&controller);
		if (stop) running = false;

		ProcessXInput(&oldController, &controller);

		gameCode.UpdateAndRenderGame(&controller, &memory, &platformCode, deltaTime);

		SwapBuffers(deviceContext);

		FrameWipe();

		lastPerfCounter = newPerfCounter;
	}

	VirtualFree(memory.frameMem, 0, MEM_RELEASE);
	VirtualFree(memory.stackMem, 0, MEM_RELEASE);
	VirtualFree(memory.transientMem, 0, MEM_RELEASE);

	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(glContext);
	ReleaseDC(windowHandle, deviceContext);
	DestroyWindow(windowHandle);
}

int WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
	(void) prevInstance, cmdLine, showCmd;
	Win32Start(hInstance);

	return 0;
}
