inline void ChangeExtension(char *buffer, const char *newExtension)
{
	char *lastDot = 0;
	for (char *scan = buffer; *scan; ++scan)
		if (*scan == '.')
			lastDot = scan;
	ASSERT(lastDot);
	strcpy(lastDot + 1, newExtension);
}
