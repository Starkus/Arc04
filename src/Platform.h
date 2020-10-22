struct Button
{
	bool endedDown;
	bool changed;
};

union Controller
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
		Button debugUp;
		Button debugDown;
		Button debugLeft;
		Button debugRight;
#endif
	};
	Button b[13];
};

struct GameMemory
{
	void *frameMem, *stackMem, *transientMem;
	void *framePtr, *stackPtr, *transientPtr;
};
