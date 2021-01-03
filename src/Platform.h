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

struct FileHandle
{
	u8 reserved[8];
};

/*
#if TARGET_WINDOWS
typedef HANDLE FileHandle;
#else
typedef int FileHandle;
#endif
*/
