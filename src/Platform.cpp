#include <windows.h>
#include <strsafe.h>
#include <GL/gl.h>
#include <GLES3/gl32.h>
#include <GLES3/glext.h>
#include <GLES3/wglext.h>

#include "General.h"
#include "Maths.h"
#include "Geometry.h"
#include "OpenGL.h"
#include "Memory.h"
#include "Platform.h"

HANDLE hStdout;

PLATFORM_LOG(Log)
{
	char buffer[256];
	va_list args;
	va_start(args, format);

	StringCbVPrintfA(buffer, 512, format, args);
	OutputDebugStringA(buffer);

	DWORD bytesWritten;
	WriteFile(hStdout, buffer, (DWORD)strlen(buffer), &bytesWritten, nullptr);

	va_end(args);
}

#include "OpenGL.cpp"
#include "Render.cpp"

// OpenGL
typedef bool (GLAPIENTRY *wglChoosePixelFormatARBProc)(HDC hdc, const int *piAttribIList,
		const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC (GLAPIENTRY *wglCreateContextAttribsARBProc)(HDC hDC, HGLRC hShareContext, const int *attribList);

wglChoosePixelFormatARBProc wglChoosePixelFormatARBPointer;
wglCreateContextAttribsARBProc wglCreateContextAttribsARBPointer;

#define wglChoosePixelFormatARB wglChoosePixelFormatARBPointer
#define wglCreateContextAttribsARB wglCreateContextAttribsARBPointer

void LoadWGLProcs()
{
	wglChoosePixelFormatARB = (wglChoosePixelFormatARBProc) GL_GetProcAddress("wglChoosePixelFormatARB");
	wglCreateContextAttribsARB = (wglCreateContextAttribsARBProc) GL_GetProcAddress("wglCreateContextAttribsARB");
}
/////////

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile)
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
	auto error = GetLastError();
	ASSERT(file != INVALID_HANDLE_VALUE);

	DWORD fileSize = GetFileSize(file, nullptr);
	ASSERT(fileSize);
	error = GetLastError();

	u8 *fileBuffer = (u8 *)malloc(fileSize);
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
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	WNDCLASSEX windowClass = {};
	ZeroMemory(&windowClass, sizeof(windowClass));
	windowClass.cbSize = sizeof(windowClass);
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = WndProc;
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

	// Load game code
	StartGame_t *StartGame;
	UpdateAndRenderGame_t *UpdateAndRenderGame;
	{
		HMODULE gameDll = LoadLibraryA("game_debug.dll");
		ASSERT(gameDll);
		StartGame = (StartGame_t *)GetProcAddress(gameDll, "StartGame");
		ASSERT(StartGame);
		UpdateAndRenderGame = (UpdateAndRenderGame_t *)GetProcAddress(gameDll, "UpdateAndRenderGame");
		ASSERT(UpdateAndRenderGame);
	}

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

	// Pass functions
	PlatformCode platformCode;
	platformCode.Log = Log;
	platformCode.PlatformReadEntireFile = PlatformReadEntireFile;
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

	StartGame(&gameMemory, &platformCode);

	bool running = true;
	while (running)
	{
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

		UpdateAndRenderGame(&controller, deltaTime);

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
