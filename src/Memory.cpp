#include "Memory.h"

// @Cleanup: This whole thing is pretty dumb

// FRAME
void *FrameAlloc(u64 size)
{
	ASSERT((u8 *)g_gameMemory->framePtr + size < (u8 *)g_gameMemory->frameMem + frameSize); // Out of memory!
	void *result;

	result = g_gameMemory->framePtr;
	g_gameMemory->framePtr = (u8 *)g_gameMemory->framePtr + size;

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
	g_gameMemory->framePtr = g_gameMemory->frameMem;
}

// STACK
void *StackAlloc(u64 size)
{
	ASSERT((u8 *)g_gameMemory->stackPtr + size < (u8 *)g_gameMemory->stackMem + stackSize); // Out of memory!
	void *result;

	result = g_gameMemory->stackPtr;
	g_gameMemory->stackPtr = (u8 *)g_gameMemory->stackPtr + size;

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
	g_gameMemory->stackPtr = ptr;
}

// TRANSIENT
void *TransientAlloc(u64 size)
{
	ASSERT((u8 *)g_gameMemory->transientPtr + size < (u8 *)g_gameMemory->transientMem + transientSize); // Out of memory!
	void *result;

	result = g_gameMemory->transientPtr;
	g_gameMemory->transientPtr = (u8 *)g_gameMemory->transientPtr + size;

	return result;
}

