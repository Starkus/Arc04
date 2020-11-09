#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GLES3/gl32.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/limits.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>

#include "General.h"
#include "Containers.h"
#include "MemoryAlloc.h"
#include "Maths.h"
#include "Render.h"
#include "Geometry.h"
#include "Resource.h"
#include "Game.h"
#include "Platform.h"
#include "PlatformCode.h"

#include "LinuxCommon.cpp"
#include "OpenGLRender.cpp"

DECLARE_ARRAY(Resource);

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

#include "BakeryInterop.cpp"
#include "Resource.cpp"

#if DEBUG_BUILD
const char *gameLibName = "./bin/Game_debug.so";
#else
const char *gameLibName = "./bin/Game.so";
#endif

GET_RESOURCE(GetResource)
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

int main(int argc, char **argv)
{
	(void) argc, argv;

	StartGame_t *StartGame;
	UpdateAndRenderGame_t *UpdateAndRenderGame;
	{
		void *gameLib = dlopen(gameLibName, RTLD_NOW);
		if (gameLib == nullptr)
		{
			printf("Couldn't load game library! Error %s\n", dlerror());
			return 3;
		}
		StartGame = (StartGame_t*)dlsym(gameLib, "StartGame");
		ASSERT(StartGame != nullptr);
		UpdateAndRenderGame = (UpdateAndRenderGame_t*)dlsym(gameLib, "UpdateAndRenderGame");
		ASSERT(UpdateAndRenderGame != nullptr);
	}

	Display *display;
	Window window;
	GLXContext glContext;

	display = XOpenDisplay(nullptr);
	if (display == nullptr)
	{
		printf("ERROR! Cannot connect to X server!\n");
		return 1;
	}

	Window rootWindow = DefaultRootWindow(display);

	GLint attr[] =
	{
		GLX_RGBA,
		GLX_DEPTH_SIZE, 24,
		GLX_DOUBLEBUFFER,
		None
	};
	XVisualInfo *visualInfo = glXChooseVisual(display, 0, attr);
	if (visualInfo == nullptr)
	{
		printf("ERROR! No appropriate visual found!\n");
		return 2;
	}
	printf("Visual %p selected.\n", (void *)visualInfo->visualid);

	Colormap colorMap = XCreateColormap(display, rootWindow, visualInfo->visual, AllocNone);
	XSetWindowAttributes setWindowAttr = {};
	setWindowAttr.colormap = colorMap;
	setWindowAttr.event_mask = ExposureMask | KeyPressMask;

	window = XCreateWindow(display, rootWindow, 0, 0, 800, 600, 0, visualInfo->depth, InputOutput,
			visualInfo->visual, CWColormap | CWEventMask, &setWindowAttr);

	XMapWindow(display, window);
	XStoreName(display, window, "3DVania");

	XSelectInput(display, window, KeyPressMask | KeyReleaseMask);
	glContext = glXCreateContext(display, visualInfo, nullptr, GL_TRUE);
	glXMakeCurrent(display, window, glContext);

	Memory memory = {};
	g_memory = &memory;
	const int prot = PROT_READ | PROT_WRITE;
	const int flags = MAP_ANONYMOUS;
	memory.frameMem = mmap((void *)0x1000000000000000, Memory::frameSize, prot, flags, 0, 0);
	memory.stackMem = mmap((void *)0x2000000000000000, Memory::stackSize, prot, flags, 0, 0);
	memory.transientMem = mmap((void *)0x3000000000000000, Memory::transientSize, prot, flags, 0, 0);
	memory.buddyMem = mmap((void *)0x4000000000000000, Memory::buddySize, prot, flags, 0, 0);

	const u32 maxNumOfBuddyBlocks = Memory::buddySize / Memory::buddySmallest;
	memory.buddyBookkeep = (u8 *)malloc( maxNumOfBuddyBlocks);
	MemoryInit(&memory);

	Array_Resource resources;
	ArrayInit_Resource(&resources, 256, malloc);
	g_resources = &resources;

	PlatformCode platformCode = {};
	platformCode.Log = Log;
	platformCode.PlatformReadEntireFile = PlatformReadEntireFile;
	platformCode.SetUpDevice = SetUpDevice;
	platformCode.ClearBuffers = ClearBuffers;
	platformCode.GetUniform = GetUniform;
	platformCode.UseProgram = UseProgram;
	platformCode.UniformMat4 = UniformMat4;
	platformCode.UniformInt = UniformInt;
	platformCode.RenderIndexedMesh = RenderIndexedMesh;
	platformCode.RenderMesh = RenderMesh;
	platformCode.RenderLines = RenderLines;
	platformCode.CreateDeviceMesh = CreateDeviceMesh;
	platformCode.CreateDeviceIndexedMesh = CreateDeviceIndexedMesh;
	platformCode.CreateDeviceTexture = CreateDeviceTexture;
	platformCode.SendMesh = SendMesh;
	platformCode.SendIndexedMesh = SendIndexedMesh;
	platformCode.SendTexture = SendTexture;
	platformCode.BindTexture = BindTexture;
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
	platformCode.ResourceLoadTexture = ResourceLoadTexture;
	platformCode.GetResource = GetResource;

	StartGame(&memory, &platformCode);

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

		while (XPending(display))
		{
			XEvent xEvent;
			XNextEvent(display, &xEvent);
			if (xEvent.type == KeyPress || xEvent.type == KeyRelease)
			{
				bool endedDown = xEvent.type == KeyPress;
				XKeyEvent keyEvent = xEvent.xkey;

				auto checkKey = [keyEvent, endedDown, display](Button &button, KeySym keySym)
				{
					if (keyEvent.keycode == XKeysymToKeycode(display, keySym))
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

				if (keyEvent.keycode == XKeysymToKeycode(display, XK_Q))
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

		XWindowAttributes windowAttr;
		XGetWindowAttributes(display, window, &windowAttr);
		SetViewport(0, 0, windowAttr.width, windowAttr.height);

		UpdateAndRenderGame(&controller, &memory, &platformCode, deltaTime);

		glXSwapBuffers(display, window);
	}

	glXMakeCurrent(display, None, nullptr);
	glXDestroyContext(display, glContext);
	XDestroyWindow(display, window);
	XCloseDisplay(display);

	return 0;
}
