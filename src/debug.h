#if DEBUG
void Win32_Log(const char *format, char *filePath, int fileLine, char *function, ...);
#define Log(format, ...) Win32_Log(format, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define Log(format, ...) ;
#endif

