#include "Memory.h"

// @Cleanup: This whole thing is pretty dumb

// FRAME
void *frameMem;
void *framePtr;
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
	framePtr = frameMem;
}

// STACK
void *stackMem;
void *stackPtr;
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
	//Log("WARNING: STACK REALLOC\n");

	void *newBlock = StackAlloc(newSize);
	memcpy(newBlock, ptr, newSize);
	return newBlock;
}
void StackFree(void *ptr)
{
	stackPtr = ptr;
}

// TRANSIENT
void *transientMem;
void *transientPtr;
void *TransientAlloc(u64 size)
{
	ASSERT((u8 *)transientPtr + size < (u8 *)transientMem + transientSize); // Out of memory!
	void *result;

	result = transientPtr;
	transientPtr = (u8 *)transientPtr + size;

	return result;
}

