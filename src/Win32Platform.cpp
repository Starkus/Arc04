#include <windows.h>
#include <strsafe.h>
#include <GL/gl.h>
#include <GLES3/gl32.h>
#include <GLES3/glext.h>
#include <GLES3/wglext.h>
#include <GLES3/glcorearb.h>
#include <Xinput.h>

#include "OpenGL.h"

#ifdef USING_IMGUI
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include <imgui/imgui_impl_win32.cpp>
#include <imgui/imgui_impl_opengl3.cpp>
#include <imgui/imgui.cpp>
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui_widgets.cpp>
#endif

#include "General.h"
#include "MemoryAlloc.h"
#include "Containers.h"
#include "Maths.h"
#include "Render.h"
#include "Geometry.h"
#include "Resource.h"
#include "Platform.h"
#include "PlatformCode.h"
#include "Game.h"

DECLARE_ARRAY(Resource);
DECLARE_ARRAY(FILETIME);

struct ResourceBank
{
	Array_Resource resources;
	Array_FILETIME lastWriteTimes;
};

#define LOG(...) Log(__VA_ARGS__)

HANDLE g_hStdout;
Memory *g_memory;
ResourceBank *g_resourceBank;
#ifdef USING_IMGUI
ImGuiTextBuffer *g_imguiLogBuffer;
bool g_showConsole;
#endif
u32 g_windowWidth = 1440;
u32 g_windowHeight = 810;

#include "Win32Common.cpp"
#include "OpenGL.cpp"
#include "OpenGLRender.cpp"
#include "MemoryAlloc.cpp"

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

struct Win32Context
{
	HINSTANCE hInstance;
	WNDCLASSEX windowClass;
	HDC deviceContext;
	HGLRC glContext;
	HWND windowHandle;
};

#ifdef USING_IMGUI
PLATFORM_GET_IMGUI_CONTEXT(PlatformGetImguiContext)
{
	return GImGui;
}
#endif

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
					case VK_OEM_3:
						if (isDown && !wasDown)
							g_showConsole = !g_showConsole;
						break;
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
#ifdef USING_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;
#endif

	switch(message)
	{
	case WM_CLOSE:
	{
		PostQuitMessage(0);
	} break;
	case WM_SIZE:
	{
		g_windowWidth = LOWORD(lParam);
		g_windowHeight = HIWORD(lParam);
		SetViewport(0, 0, g_windowWidth, g_windowHeight);
	}break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
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

#include "Resource.cpp"

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
	DWORD fileSize;
	DWORD error = Win32ReadEntireFile(resource->filename, &fileBuffer, &fileSize, FrameAlloc);
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

		SendIndexedMesh(&resource->mesh.deviceMesh, vertexData, vertexCount, sizeof(Vertex),
				indexData, indexCount, false);

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
		SendIndexedMesh(&resource->skinnedMesh.deviceMesh, vertexData, vertexCount,
				sizeof(SkinnedVertex), indexData, indexCount, false);

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

void InitOpenGLContext(Win32Context *context)
{
	AttachConsole(ATTACH_PARENT_PROCESS);
	g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	WNDCLASSEX windowClass = {};
	ZeroMemory(&windowClass, sizeof(windowClass));
	windowClass.cbSize = sizeof(windowClass);
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = Win32WindowCallback;
	windowClass.hInstance = context->hInstance;
	windowClass.lpszClassName = "window";
	context->windowClass = windowClass;

	bool success = RegisterClassEx(&context->windowClass);
	ASSERT(success);

	// Fake window
	HWND fakeWindow = CreateWindow("window", "Fake window", WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			0, 0, 1, 1, nullptr, nullptr, context->hInstance, nullptr);

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

	context->windowHandle = CreateWindowA("window", "3DVania",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, 16, 16, g_windowWidth, g_windowHeight, 0, 0, context->hInstance, 0);


	context->deviceContext = GetDC(context->windowHandle);
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
	success = wglChoosePixelFormatARB(context->deviceContext, pixelAttribs, nullptr, 1,
			&pixelFormatId, &numFormats);
	ASSERT(success);

	DescribePixelFormat(context->deviceContext, pixelFormatId, sizeof(pfd), &pfd);
	SetPixelFormat(context->deviceContext, pixelFormatId, &pfd);

	int contextAttribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	context->glContext = wglCreateContextAttribsARB(context->deviceContext, 0, contextAttribs);
	ASSERT(context->glContext);

	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(fakeGlContext);
	ReleaseDC(fakeWindow, fakeContext);
	DestroyWindow(fakeWindow);

	success = wglMakeCurrent(context->deviceContext, context->glContext);
	ASSERT(success);
	context->glContext;
}

void Win32Start(HINSTANCE hInstance)
{
	Win32Context context;
	context.hInstance = hInstance;

	// Allocate memory
	Memory memory;
	g_memory = &memory;
	memory.frameMem = VirtualAlloc(0, Memory::frameSize, MEM_COMMIT, PAGE_READWRITE);
	memory.stackMem = VirtualAlloc(0, Memory::stackSize, MEM_COMMIT, PAGE_READWRITE);
	memory.transientMem = VirtualAlloc(0, Memory::transientSize, MEM_COMMIT, PAGE_READWRITE);
	memory.buddyMem = VirtualAlloc(0, Memory::buddySize, MEM_COMMIT, PAGE_READWRITE);

	const u32 maxNumOfBuddyBlocks = Memory::buddySize / Memory::buddySmallest;
	memory.buddyBookkeep = (u8 *)VirtualAlloc(0, maxNumOfBuddyBlocks, MEM_COMMIT, PAGE_READWRITE);
	MemoryInit(&memory);

	InitOpenGLContext(&context);

#ifdef USING_IMGUI
	// Setup imgui
	ImGui::SetAllocatorFunctions(BuddyAlloc, BuddyFree);
	ImGuiTextBuffer logBuffer;
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui_ImplWin32_Init(context.windowHandle);

		const char* glslVersion = "#version 330";
		ImGui_ImplOpenGL3_Init(glslVersion);

		g_imguiLogBuffer = &logBuffer;
	}
#endif

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

	ResourceBank resourceBank;
	ArrayInit_Resource(&resourceBank.resources, 256, malloc);
	ArrayInit_FILETIME(&resourceBank.lastWriteTimes, 256, malloc);
	g_resourceBank = &resourceBank;

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
	platformCode.RenderMeshInstanced = RenderMeshInstanced;
	platformCode.RenderIndexedMeshInstanced = RenderIndexedMeshInstanced;
	platformCode.RenderLines = RenderLines;
	platformCode.CreateDeviceMesh = CreateDeviceMesh;
	platformCode.CreateDeviceIndexedMesh = CreateDeviceIndexedMesh;
	platformCode.SendMesh = SendMesh;
	platformCode.SendIndexedMesh = SendIndexedMesh;
	platformCode.CreateShader = CreateShader;
	platformCode.LoadShader = LoadShader;
	platformCode.AttachShader = AttachShader;
	platformCode.CreateDeviceProgram = CreateDeviceProgram;
	platformCode.LinkDeviceProgram = LinkDeviceProgram;
	platformCode.SetViewport = SetViewport;
	platformCode.SetFillMode = SetFillMode;
	platformCode.ResourceLoadMesh = ResourceLoadMesh;
	platformCode.ResourceLoadSkinnedMesh = ResourceLoadSkinnedMesh;
	platformCode.ResourceLoadLevelGeometryGrid = ResourceLoadLevelGeometryGrid;
	platformCode.ResourceLoadPoints = ResourceLoadPoints;
	platformCode.ResourceLoadShader = ResourceLoadShader;
	platformCode.GetResource = GetResource;
#ifdef USING_IMGUI
	platformCode.PlatformGetImguiContext = PlatformGetImguiContext;
#endif

	gameCode.StartGame(&memory, &platformCode);

	bool running = true;
	while (running)
	{
		QueryPerformanceCounter(&largeInteger);
		const u64 newPerfCounter = largeInteger.LowPart;
		const f64 time = (f64)newPerfCounter / (f64)perfFrequency;
		const f32 deltaTime = (f32)(newPerfCounter - lastPerfCounter) / (f32)perfFrequency;

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

		// Check events
		{
			Controller oldController = controller;
			for (int buttonIdx = 0; buttonIdx < ArrayCount(controller.b); ++buttonIdx)
				controller.b[buttonIdx].changed = false;

			bool stop = ProcessKeyboard(&controller);
			if (stop) running = false;

			ProcessXInput(&oldController, &controller);
		}

#ifdef USING_IMGUI
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
#endif

		gameCode.UpdateAndRenderGame(&controller, &memory, &platformCode, deltaTime);

#ifdef USING_IMGUI
		if (g_showConsole)
		{
			// In game console
			ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2((f32)g_windowWidth, 250), ImGuiCond_Always);

			if (ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoTitleBar |
						ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
						ImGuiWindowFlags_NoCollapse))
			{
				static bool autoScroll = true;
				ImGui::Checkbox("Auto-scroll", &autoScroll);

				ImGui::Separator();

				ImGui::BeginChild("ConsoleScrollingRegion", ImVec2(0, 0), false,
						ImGuiWindowFlags_HorizontalScrollbar);
				const char *lineStart = g_imguiLogBuffer->begin();
				for (const char *scan = g_imguiLogBuffer->begin();
						scan != g_imguiLogBuffer->end();
						++scan)
				{
					if (*scan == '\n')
					{
						ImVec4 color;
						bool hasColor = false;
						if (strncmp(lineStart, "ERROR", 5) == 0)
						{
							color = ImVec4(0.6f, 0.1f, 0.1f, 1.0f);
							hasColor = true;
						}
						else if (strncmp(lineStart, "WARNING", 7) == 0)
						{
							color = ImVec4(0.4f, 0.3f, 0.1f, 1.0f);
							hasColor = true;
						}
						if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, color);
						ImGui::TextUnformatted(lineStart, scan);
						if (hasColor) ImGui::PopStyleColor();
						lineStart = scan + 1;
					}
				}
				if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
					ImGui::SetScrollHereY(1.0f);

				ImGui::EndChild();
			}
			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

		SwapBuffers(context.deviceContext);

		FrameWipe();

		lastPerfCounter = newPerfCounter;
	}

	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(context.glContext);
	ReleaseDC(context.windowHandle, context.deviceContext);
	DestroyWindow(context.windowHandle);

	Win32UnloadGameCode(&gameCode);
}

int WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
	(void) prevInstance, cmdLine, showCmd;
	Win32Start(hInstance);

	return 0;
}
