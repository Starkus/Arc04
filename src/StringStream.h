struct StringStream
{
	char *buffer;
	u64 capacity;
	u64 cursor;
};

inline void StreamWrite(StringStream *stream, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	ASSERT(stream->cursor < stream->capacity);
	stream->cursor += vsprintf(stream->buffer + stream->cursor, format, args);
	ASSERT(stream->cursor < stream->capacity);

	va_end(args);
}
