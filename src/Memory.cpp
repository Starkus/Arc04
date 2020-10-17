// FRAME
const u64 frameSize = 256 * 1024 * 1024;
void *frameMem;
void *framePtr;
void FrameInit()
{
	frameMem = malloc(frameSize);
	ASSERT(frameSize);
	SDL_Log("Allocated %.02fmb of frame memory (%d bytes)\n", frameSize /
			(1024.0f * 1024.0f), frameSize);
	framePtr = frameMem;
}
void FrameFinalize()
{
	free(frameMem);
}
void *FrameAlloc(u64 size)
{
	ASSERT((u8 *)framePtr + size < (u8 *)frameMem + frameSize); // Out of memory!
	void *result;

	result = framePtr;
	framePtr = (u8 *)framePtr + size;

	return result;
}
void *FrameRealloc(void *ptr, u64 newSize)
{
	//SDL_Log("WARNING: FRAME REALLOC\n");

	void *newBlock = FrameAlloc(newSize);
	memcpy(newBlock, ptr, newSize);
	return newBlock;
}
void FrameFree(void *ptr)
{
	(void) ptr;
}
void FrameWipe()
{
	framePtr = frameMem;
}

// STACK
const u64 stackSize = 128 * 1024 * 1024;
void *stackMem;
void *stackPtr;
void StackInit()
{
	stackMem = malloc(stackSize);
	ASSERT(stackMem);
	SDL_Log("Allocated %.02fmb of stack memory (%d bytes)\n", stackSize /
			(1024.0f * 1024.0f), stackSize);
	stackPtr = stackMem;
}
void StackFinalize()
{
	free(stackMem);
}
void *StackAlloc(u64 size)
{
	ASSERT((u8 *)stackPtr + size < (u8 *)stackMem + stackSize); // Out of memory!
	void *result;

	result = stackPtr;
	stackPtr = (u8 *)stackPtr + size;

	return result;
}
void *StackRealloc(void *ptr, u64 newSize)
{
	//ASSERT(false);
	//SDL_Log("WARNING: STACK REALLOC\n");

	void *newBlock = StackAlloc(newSize);
	memcpy(newBlock, ptr, newSize);
	return newBlock;
}
void StackFree(void *ptr)
{
	stackPtr = ptr;
}

