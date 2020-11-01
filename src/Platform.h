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
			Button debugUp;
			Button debugDown;
			Button debugLeft;
			Button debugRight;
#endif
		};
		Button b[13];
	};
};

struct PlatformFileTime
{
	u8 reserved[8];
};

struct PlatformFindData
{
	u8 reserved[8];
};

struct PlatformSearchHandle
{
	u8 reserved[8];
};
