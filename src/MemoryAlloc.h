struct Memory
{
	void *frameMem, *stackMem, *transientMem;
	void *framePtr, *stackPtr, *transientPtr;
};

const u64 frameSize = 256 * 1024 * 1024;
const u64 stackSize = 128 * 1024 * 1024;
const u64 transientSize = 512 * 1024 * 1024;
