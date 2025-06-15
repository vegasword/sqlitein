#if DEBUG
void Win32_InitDebugConsole()
{
  AllocConsole();
  AttachConsole(GetCurrentProcessId());
  HANDLE logger = GetStdHandle(STD_OUTPUT_HANDLE);
  SetStdHandle(STD_OUTPUT_HANDLE, logger);
  SetStdHandle(STD_ERROR_HANDLE, logger);
}

void Win32_Log(const char *format, char *filePath, int fileLine, char *function, ...)
{    
  HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
  
  char log[4*KB];
  char *fileName = filePath + strnlen_s(filePath, MAX_PATH);
  while (*(fileName - 1) != '\\') fileName--;
  sprintf_s(log, sizeof(log), "%s(%d) | %s : ", fileName, fileLine, function);
  WriteFile(console, log, (DWORD)strlen(log), 0, NULL);
  OutputDebugString(log);
  
  char message[4*KB];
  va_list args;
  va_start(args, function);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);
  WriteFile(console, message, (DWORD)strlen(message), 0, NULL);
  OutputDebugString(message);
}
#endif

void Win32_Quit(Win32Context *context)
{
#if DEBUG
  SetStdHandle(STD_OUTPUT_HANDLE, NULL);
  SetStdHandle(STD_ERROR_HANDLE, NULL);
  FreeConsole();
#endif
  context->quitting = true;
  ReleaseDC(context->window, context->dc);
  wglDeleteContext(context->glrc);
}

File Win32_ReadWholeFileEx(Arena *arena, const char* path, DWORD desiredAccess, DWORD sharedMode, DWORD creationDisposition, DWORD flags)
{
  File file = {0};
  file.handle = CreateFile(path, desiredAccess, sharedMode, NULL, creationDisposition, flags, NULL);
  assert(file.handle != INVALID_HANDLE_VALUE);
  
  LARGE_INTEGER largeSize = {0};
  GetFileSizeEx(file.handle, &largeSize);

  DWORD numberOfBytesToRead = (DWORD)largeSize.QuadPart;
  DWORD numberOfBytesRead = 0;
  
  TmpArena tmpArena = {0};
  TmpBegin(&tmpArena, arena);
  
  file.data = (uc *)Alloc(arena, numberOfBytesToRead);
  file.size = numberOfBytesToRead;

  BOOL result = ReadFile(file.handle, file.data, numberOfBytesToRead, &numberOfBytesRead, NULL);
  assert(result && numberOfBytesToRead == numberOfBytesRead);

  file.data[numberOfBytesRead] = '\0';
  
  TmpEnd(&tmpArena);
  CloseHandle(file.handle);

  return file;
}
#define Win32_ReadWholeFile(arena, path) Win32_ReadWholeFileEx(arena, path, GENERIC_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY)

i32 Win32_OpenFileDialog(Win32Context *context, char *outFilePath)
{  
  OPENFILENAME openFileName = {
    .lStructSize = sizeof(OPENFILENAME),
    .hwndOwner = context->window,
    .lpstrFile = outFilePath,
    .nMaxFile = MAX_PATH,
    .lpstrFilter = "*.db\0",
    .nFilterIndex = 1,
    .Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
  };
    
  return GetOpenFileName(&openFileName) ? 1 : CommDlgExtendedError();
}
