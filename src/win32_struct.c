typedef struct {
  LARGE_INTEGER frequency;
  LARGE_INTEGER t1;
  LARGE_INTEGER t2;
  u64 elapsedMilliseconds;
} PerfCounter;

typedef struct File {
  HANDLE handle;
  uc *buffer;
} File;

typedef struct {
  GLint program;
  GLenum type;
#if DEBUG
  HANDLE fileHandle;
  FILETIME lastWriteTime;
#endif
} Shader;

typedef struct {
  HDC dc;
  HGLRC glrc;
  HWND window;
  u32 windowWidth;
  u32 windowHeight;
  bool quitting;
} Win32Context;
