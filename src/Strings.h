inline void ChangeExtension(char *buffer, const char *newExtension)
{
	char *lastDot = 0;
	for (char *scan = buffer; *scan; ++scan)
		if (*scan == '.')
			lastDot = scan;
	do { if (!(lastDot)) __debugbreak(); } while (false);
	strcpy(lastDot + 1, newExtension);
}
