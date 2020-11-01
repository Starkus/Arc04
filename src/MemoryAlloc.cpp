// @Cleanup: This whole thing is pretty dumb

// FRAME
void *FrameAlloc(u64 size)
{
	ASSERT((u8 *)g_memory->framePtr + size < (u8 *)g_memory->frameMem + frameSize); // Out of memory!
	void *result;

	result = g_memory->framePtr;
	g_memory->framePtr = (u8 *)g_memory->framePtr + size;

	return result;
}
void *FrameRealloc(void *ptr, u64 newSize)
{
	//Log("WARNING: FRAME REALLOC\n");

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
	g_memory->framePtr = g_memory->frameMem;
}

// STACK
void *StackAlloc(u64 size)
{
	ASSERT((u8 *)g_memory->stackPtr + size < (u8 *)g_memory->stackMem + stackSize); // Out of memory!
	void *result;

	result = g_memory->stackPtr;
	g_memory->stackPtr = (u8 *)g_memory->stackPtr + size;

	return result;
}
void *StackRealloc(void *ptr, u64 newSize)
{
	//ASSERT(false);
	//Log("WARNING: STACK REALLOC\n");

	void *newBlock = StackAlloc(newSize);
	memcpy(newBlock, ptr, newSize);
	return newBlock;
}
void StackFree(void *ptr)
{
	g_memory->stackPtr = ptr;
}

// TRANSIENT
void *TransientAlloc(u64 size)
{
	ASSERT((u8 *)g_memory->transientPtr + size < (u8 *)g_memory->transientMem + transientSize); // Out of memory!
	void *result;

	result = g_memory->transientPtr;
	g_memory->transientPtr = (u8 *)g_memory->transientPtr + size;

	return result;
}

