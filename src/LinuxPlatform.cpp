@Ignore #include <stdio.h>
@Ignore #include <stdlib.h>
@Ignore #include <string.h>
@Ignore #include <dlfcn.h>
@Ignore #include <fcntl.h>
@Ignore #include <unistd.h>
@Ignore #include <sys/stat.h>
@Ignore #include <sys/mman.h>
@Ignore #include <linux/limits.h>
@Ignore #include <time.h>
@Ignore #include <errno.h>
@Ignore #include <dirent.h>

@Ignore #include <X11/X.h>
@Ignore #include <X11/Xlib.h>
@Ignore #include <GL/gl.h>
@Ignore #include <GL/glx.h>
@Ignore #include <GL/glu.h>
@Ignore #include <GLES3/gl32.h>

#include "General.h"
#include "Containers.h"
#include "MemoryAlloc.h"
#include "Maths.h"
#include "Render.h"
#include "Geometry.h"
#include "Resource.h"
#include "Platform.h"
@Ignore #include "PlatformCode.h"
#include "GameInterface.h"

#include "LinuxCommon.cpp"
#include "RenderOpenGL.cpp"

void Log(const char *format, ...) @PlatformProc;
bool PlatformReadEntireFile(const char *filename, u8 **fileBuffer, u64 *fileSize,
		void *(*allocFunc)(u64)) @PlatformProc;

DECLARE_ARRAY(Resource);

struct LinuxGameCode
{
	StartGame_t *StartGame;
	GameResourcePostLoad_t *GameResourcePostLoad;
	InitGameModule_t *InitGameModule;
	UpdateAndRenderGame_t *UpdateAndRenderGame;
};

struct LinuxContext
{
	Display *display;
	Window window;
	GLXContext glContext;
};

LinuxGameCode *g_gameCode;
LinuxContext *g_linuxContext;

Memory *g_memory;
Array_Resource *g_resources;

#include "MemoryAlloc.cpp"

Resource *CreateResource(const char *filename)
{
	Resource result = {};
	strcpy(result.filename, filename);

	Resource *resource = &(*g_resources)[g_resources->size++];
	*resource = result;
	return resource;
}

const Resource *LoadResource(ResourceType type, const char *filename) @PlatformProc
{
	Resource *newResource = CreateResource(filename);

	u8 *fileBuffer;
	u64 fileSize;
	int error = LinuxReadEntireFile(filename, &fileBuffer, &fileSize, FrameAlloc);
	if (error != 0)
		return nullptr;

	newResource->type = type;
	g_gameCode->GameResourcePostLoad(newResource, fileBuffer, true);

	return newResource;
}

const Resource *GetResource(const char *filename) @PlatformProc
{
	for (u32 i = 0; i < g_resources->size; ++i)
	{
		Resource *it = &(*g_resources)[i];
		if (strcmp(it->filename, filename) == 0)
		{
			return it;
		}
	}
	return nullptr;
}

void GetWindowSize(u32 *w, u32 *h) @PlatformProc
{
	XWindowAttributes windowAttr;
	XGetWindowAttributes(g_linuxContext->display, g_linuxContext->window, &windowAttr);
	*w = windowAttr.width;
	*h = windowAttr.height;
}

#include "PlatformCode.cpp"

#if DEBUG_BUILD
const char *gameLibName = "./bin/Game_debug.so";
#else
const char *gameLibName = "./bin/Game.so";
#endif

int main(int argc, char **argv)
{
	(void) argc, argv;

	LinuxContext context;
	LinuxGameCode gameCode;
	g_linuxContext = &context;
	g_gameCode = &gameCode;

	// Load game code
	{
		void *gameLib = dlopen(gameLibName, RTLD_NOW);
		if (gameLib == nullptr)
		{
			printf("Couldn't load game library! Error %s\n", dlerror());
			return 3;
		}
		gameCode.InitGameModule = (InitGameModule_t*)dlsym(gameLib, "InitGameModule");
		ASSERT(gameCode.InitGameModule != nullptr);
		gameCode.GameResourcePostLoad = (GameResourcePostLoad_t*)dlsym(gameLib, "GameResourcePostLoad");
		ASSERT(gameCode.GameResourcePostLoad != nullptr);
		gameCode.StartGame = (StartGame_t*)dlsym(gameLib, "StartGame");
		ASSERT(gameCode.StartGame != nullptr);
		gameCode.UpdateAndRenderGame = (UpdateAndRenderGame_t*)dlsym(gameLib, "UpdateAndRenderGame");
		ASSERT(gameCode.UpdateAndRenderGame != nullptr);
	}

	context.display = XOpenDisplay(nullptr);
	if (context.display == nullptr)
	{
		printf("ERROR! Cannot connect to X server!\n");
		return 1;
	}

	Window rootWindow = DefaultRootWindow(context.display);

	GLint attr[] =
	{
		GLX_RGBA,
		GLX_DEPTH_SIZE, 24,
		GLX_DOUBLEBUFFER,
		None
	};
	XVisualInfo *visualInfo = glXChooseVisual(context.display, 0, attr);
	if (visualInfo == nullptr)
	{
		printf("ERROR! No appropriate visual found!\n");
		return 2;
	}
	printf("Visual %p selected.\n", (void *)visualInfo->visualid);

	Colormap colorMap = XCreateColormap(context.display, rootWindow, visualInfo->visual, AllocNone);
	XSetWindowAttributes setWindowAttr = {};
	setWindowAttr.colormap = colorMap;
	setWindowAttr.event_mask = ExposureMask | KeyPressMask;

	context.window = XCreateWindow(context.display, rootWindow, 0, 0, 800, 600, 0, visualInfo->depth, InputOutput,
			visualInfo->visual, CWColormap | CWEventMask, &setWindowAttr);

	XMapWindow(context.display, context.window);
	XStoreName(context.display, context.window, "3DVania");

	XSelectInput(context.display, context.window, KeyPressMask | KeyReleaseMask);
	context.glContext = glXCreateContext(context.display, visualInfo, nullptr, GL_TRUE);
	glXMakeCurrent(context.display, context.window, context.glContext);

	Memory memory = {};
	g_memory = &memory;
	const int prot = PROT_READ | PROT_WRITE;
	const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	memory.frameMem = mmap((void *)0x1000000000, Memory::frameSize, prot, flags, -1, 0);
	memory.stackMem = mmap((void *)0x2000000000, Memory::stackSize, prot, flags, -1, 0);
	memory.transientMem = mmap((void *)0x3000000000, Memory::transientSize, prot, flags, -1, 0);
	memory.buddyMem = mmap((void *)0x4000000000, Memory::buddySize, prot, flags, -1, 0);
	Log("%p %p %p %p\n", memory.frameMem, memory.stackMem, memory.transientMem, memory.buddyMem);
	Log("Error: %s\n", strerror(errno));

	const u32 maxNumOfBuddyBlocks = Memory::buddySize / Memory::buddySmallest;
	memory.buddyBookkeep = (u8 *)malloc( maxNumOfBuddyBlocks);
	MemoryInit(&memory);

	Array_Resource resources;
	ArrayInit_Resource(&resources, 256, malloc);
	g_resources = &resources;

	PlatformCode platformCode;
	FillPlatformCodeStruct(&platformCode);

	PlatformContext platformContext = {};
	platformContext.platformCode = &platformCode;
	platformContext.memory = &memory;

	gameCode.InitGameModule(platformContext);
	gameCode.StartGame();

	Controller controller = {};

	timespec lastTimeSpec;
	clock_gettime(CLOCK_MONOTONIC_RAW, &lastTimeSpec);
	bool running = true;
	while (running)
	{
		timespec newTimeSpec;
		clock_gettime(CLOCK_MONOTONIC_RAW, &newTimeSpec);
		f32 deltaTime = newTimeSpec.tv_sec - lastTimeSpec.tv_sec;
		deltaTime += Fmod((f32)(newTimeSpec.tv_nsec - lastTimeSpec.tv_nsec) / 1000000000.0f, 1.0f);
		lastTimeSpec = newTimeSpec;

		for (u32 i = 0; i < ArrayCount(controller.b); ++i)
			controller.b[i].changed = false;

		while (XPending(context.display))
		{
			XEvent xEvent;
			XNextEvent(context.display, &xEvent);
			if (xEvent.type == KeyPress || xEvent.type == KeyRelease)
			{
				bool endedDown = xEvent.type == KeyPress;
				XKeyEvent keyEvent = xEvent.xkey;

				auto checkKey = [keyEvent, endedDown](Button &button, KeySym keySym)
				{
					if (keyEvent.keycode == XKeysymToKeycode(g_linuxContext->display, keySym))
					{
						if (button.endedDown != endedDown)
						{
							button.endedDown = endedDown;
							button.changed = true;
						}
						return true;
					}
					return false;
				};

				if (keyEvent.keycode == XKeysymToKeycode(context.display, XK_Q))
				{
					running = false;
				}
				if (checkKey(controller.jump, XK_space)) continue;
				if (checkKey(controller.up, XK_w)) continue;
				if (checkKey(controller.left, XK_a)) continue;
				if (checkKey(controller.down, XK_s)) continue;
				if (checkKey(controller.right, XK_d)) continue;
				if (checkKey(controller.camUp, XK_Up)) continue;
				if (checkKey(controller.camLeft, XK_Left)) continue;
				if (checkKey(controller.camDown, XK_Down)) continue;
				if (checkKey(controller.camRight, XK_Right)) continue;
			}
		}

		{
			u32 w, h;
			GetWindowSize(&w, &h);
			SetViewport(0, 0, w, h);
		}

		gameCode.UpdateAndRenderGame(&controller, deltaTime);

		glXSwapBuffers(context.display, context.window);

		FrameWipe();
	}

	glXMakeCurrent(context.display, None, nullptr);
	glXDestroyContext(context.display, context.glContext);
	XDestroyWindow(context.display, context.window);
	XCloseDisplay(context.display);

	return 0;
}
