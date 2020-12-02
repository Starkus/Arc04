struct Button
{
	bool endedDown;
	bool changed;
};

struct Controller
{
	v2 leftStick;
	v2 rightStick;
	union
	{
		struct
		{
			Button up;
			Button down;
			Button left;
			Button right;
			Button jump;
			Button camUp;
			Button camDown;
			Button camLeft;
			Button camRight;

#if DEBUG_BUILD
			Button mouseLeft;
			Button mouseMiddle;
			Button mouseRight;

			Button debugUp;
			Button debugDown;
			Button debugLeft;
			Button debugRight;
#endif
		};
		Button b[16];
	};
	v2 mousePos;
};

struct PlatformCode;
struct ImGuiContext;
struct PlatformContext
{
	PlatformCode *platformCode;
	Memory *memory;
#if USING_IMGUI
	ImGuiContext *imguiContext;
#else
	void *imguiContext;
#endif
};

#define INIT_GAME_MODULE(name) void name(PlatformContext platformContext)
typedef INIT_GAME_MODULE(InitGameModule_t);
INIT_GAME_MODULE(InitGameModuleStub) { (void) platformContext; }

#define GAME_RESOURCE_POST_LOAD(name) bool name(Resource *resource, u8 *fileBuffer, \
		bool initialize)
typedef GAME_RESOURCE_POST_LOAD(GameResourcePostLoad_t);
GAME_RESOURCE_POST_LOAD(GameResourcePostLoadStub) { (void)resource, fileBuffer, initialize; return false; }

#define START_GAME(name) void name()
typedef START_GAME(StartGame_t);
START_GAME(StartGameStub) { }

#define UPDATE_AND_RENDER_GAME(name) void name(Controller *controller, f32 deltaTime)
typedef UPDATE_AND_RENDER_GAME(UpdateAndRenderGame_t);
UPDATE_AND_RENDER_GAME(UpdateAndRenderGameStub) { (void) controller, deltaTime; }
