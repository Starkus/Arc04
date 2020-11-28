#if PREPROCESSING
@Ignore #define PLATFORMPROC
#else
#define PLATFORMPROC
#endif

struct PlatformFileTime
{
	u8 reserved[8];
};

struct PlatformFindData
{
	u8 reserved[320];
};

struct PlatformSearchHandle
{
	u8 reserved[8];
};
