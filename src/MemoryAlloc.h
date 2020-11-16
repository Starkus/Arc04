struct Memory
{
	void *frameMem, *stackMem, *transientMem, *buddyMem;
	void *framePtr, *stackPtr, *transientPtr;
	u64 lastFrameUsage;
	u8 *buddyBookkeep;

#if DEBUG_BUILD
	// Memory usage tracking info
	u64 buddyMemoryUsage;
#endif

	static const u64 frameSize = 64 * 1024 * 1024;
	static const u64 stackSize = 8 * 1024 * 1024;
	static const u64 transientSize = 8 * 1024 * 1024;
	static const u64 buddySize = 32 * 1024 * 1024;
	static const u64 buddySmallest = 64;
	static const u32 buddyUsedBit = 0x80;
};
