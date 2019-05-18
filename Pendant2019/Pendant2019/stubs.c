
int __attribute__((weak)) _read (int file, char * ptr, int len)
{
	int nChars = 0;

	if (file != 0) {
		return -1;
	}

	for (; len > 0; --len) {
		//		ptr_get(stdio_base, ptr);
		ptr++;
		nChars++;
	}
	return nChars;
}

int __attribute__((weak)) _write (int file, const char *ptr, int len)
{
	(void) ptr;
	int nChars = 0;

	if ((file != 1) && (file != 2) && (file!=3)) {
		return -1;
	}

	for (; len != 0; --len) {
		//		if (ptr_put(stdio_base, *ptr++) < 0) {
		//			return -1;
		//		}
		++nChars;
	}
	return nChars;
}
