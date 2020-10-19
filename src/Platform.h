#define PLATFORM_READ_ENTIRE_FILE(name) u8 *name(const char *filename)
typedef PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile_t);

#define START_GAME(name) void name(GameState *gameState)
typedef START_GAME(StartGame_t);
START_GAME(StartGameStub) { (void) gameState; }

#define UPDATE_AND_RENDER_GAME(name) void name(GameState *gameState, f32 deltaTime)
typedef UPDATE_AND_RENDER_GAME(UpdateAndRenderGame_t);
UPDATE_AND_RENDER_GAME(UpdateAndRenderGameStub) { (void) gameState, deltaTime; }

