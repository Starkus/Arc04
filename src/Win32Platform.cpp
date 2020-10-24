#include <windows.h>
#include <strsafe.h>
#include <GL/gl.h>
#include <GLES3/gl32.h>
#include <GLES3/glext.h>
#include <GLES3/wglext.h>
#include <GLES3/glcorearb.h>

#include "General.h"
#include "OpenGL.h"
#include "Memory.h"
#include "Platform.h"

// Stuff referenced by platform functions
#include "Maths.h"
#include "Geometry.h"
#include "Render.h"
// The platform functions
#include "PlatformCode.h"

HANDLE g_hStdout;

PLATFORM_LOG(Log)
{
	char buffer[256];
	va_list args;
	va_start(args, format);

	StringCbVPrintfA(buffer, 512, format, args);
	OutputDebugStringA(buffer);

	DWORD bytesWritten;
	WriteFile(g_hStdout, buffer, (DWORD)strlen(buffer), &bytesWritten, nullptr);

	va_end(args);
}

#include "OpenGL.cpp"
#include "Render.cpp"

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

struct Win32GameCode
{
	HMODULE dll;
	bool isLoaded;
	FILETIME lastWriteTime;

	StartGame_t *StartGame;
	UpdateAndRenderGame_t *UpdateAndRenderGame;
};

void Win32LoadGameCode(Win32GameCode *gameCode, const char *dllFilename, const char *tempDllFilename)
{
	Log("Loading game code\n");
#if DEBUG_BUILD
	CopyFile(dllFilename, tempDllFilename, false);
	gameCode->dll = LoadLibraryA(tempDllFilename);
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

	HANDLE file = CreateFileA(
			fullname,
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
			);
	DWORD error = GetLastError();
	ASSERT(file != INVALID_HANDLE_VALUE);

	DWORD fileSize = GetFileSize(file, nullptr);
	ASSERT(fileSize);
	error = GetLastError();

	u8 *fileBuffer = (u8 *)malloc(fileSize); // @Cleanup: no malloc! Also memory leak
	DWORD bytesRead;
	bool success = ReadFile(
			file,
			fileBuffer,
			fileSize,
			&bytesRead,
			nullptr
			);
	ASSERT(success);
	ASSERT(bytesRead == fileSize);

	CloseHandle(file);

	return fileBuffer;
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
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, 16, 16, 960, 540, 0, 0, hInstance, 0);


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
	GameMemory gameMemory;
	gameMemory.frameMem = VirtualAlloc(0, frameSize, MEM_COMMIT, PAGE_READWRITE);
	gameMemory.stackMem = VirtualAlloc(0, stackSize, MEM_COMMIT, PAGE_READWRITE);
	gameMemory.transientMem = VirtualAlloc(0, transientSize, MEM_COMMIT, PAGE_READWRITE);
	gameMemory.framePtr = gameMemory.frameMem;
	gameMemory.stackPtr = gameMemory.stackMem;
	gameMemory.transientPtr = gameMemory.transientMem;

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
	platformCode.CreateDeviceMesh = CreateDeviceMesh;
	platformCode.CreateDeviceIndexedMesh = CreateDeviceIndexedMesh;
	platformCode.CreateDeviceIndexedSkinnedMesh = CreateDeviceIndexedSkinnedMesh;
	platformCode.SendMesh = SendMesh;
	platformCode.SendIndexedMesh = SendIndexedMesh;
	platformCode.SendIndexedSkinnedMesh = SendIndexedSkinnedMesh;
	platformCode.LoadShader = LoadShader;
	platformCode.CreateDeviceProgram = CreateDeviceProgram;
	platformCode.SetFillMode = SetFillMode;

	gameCode.StartGame(&gameMemory, &platformCode);

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

		QueryPerformanceCounter(&largeInteger);
		const u64 newPerfCounter = largeInteger.LowPart;
		const f64 time = (f64)newPerfCounter / (f64)perfFrequency;
		const f32 deltaTime = (f32)(newPerfCounter - lastPerfCounter) / (f32)perfFrequency;

		// Check events
		for (int buttonIdx = 0; buttonIdx < ArrayCount(controller.b); ++buttonIdx)
			controller.b[buttonIdx].changed = false;

		MSG message;
		while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
		{
			switch (message.message)
			{
				case WM_QUIT:
				{
					running = false;
				} break;
				case WM_SYSKEYDOWN:
				case WM_SYSKEYUP:
				case WM_KEYDOWN:
				case WM_KEYUP:
				{
					const bool isDown = (message.lParam & (1 << 31)) == 0;
					//const bool wasDown = (message.lParam & (1 << 30)) != 0;

					auto checkButton = [&isDown](Button &button)
					{
						if (button.endedDown != isDown)
						{
							button.endedDown = isDown;
							button.changed = true;
						}
					};

					Controller &c = controller;
					switch (message.wParam)
					{
						case 'Q':
							running = false;
							break;
						case 'W':
							checkButton(c.up);
							break;
						case 'A':
							checkButton(c.left);
							break;
						case 'S':
							checkButton(c.down);
							break;
						case 'D':
							checkButton(c.right);
							break;

						case VK_SPACE:
							checkButton(c.jump);
							break;

						case VK_UP:
							checkButton(c.camUp);
							break;
						case VK_DOWN:
							checkButton(c.camDown);
							break;
						case VK_LEFT:
							checkButton(c.camLeft);
							break;
						case VK_RIGHT:
							checkButton(c.camRight);
							break;

#if DEBUG_BUILD
						case 'H':
							checkButton(c.debugUp);
							break;
						case 'J':
							checkButton(c.debugDown);
							break;
						case 'K':
							checkButton(c.debugUp);
							break;
						case 'L':
							checkButton(c.debugDown);
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

		gameCode.UpdateAndRenderGame(&controller, &gameMemory, &platformCode, deltaTime);

		SwapBuffers(deviceContext);

		lastPerfCounter = newPerfCounter;
	}

	VirtualFree(gameMemory.frameMem, 0, MEM_RELEASE);
	VirtualFree(gameMemory.stackMem, 0, MEM_RELEASE);
	VirtualFree(gameMemory.transientMem, 0, MEM_RELEASE);

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
