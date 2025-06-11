#include "stdint.h"
#include "stdbool.h"
#include "assert.h"

#pragma warning(push)
#pragma warning(disable: 4201)
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#ifdef _WIN32
  #include "cimgui_win32_backend.h"
#endif
#pragma warning(pop)

#include "type.c"
#include "arena.c"
#include "debug.h"
#include "algorithm.c"

#define WINDOW_TITLE "SQLitein"
#define SQLITE_ENABLE_MEMSYS5
#define SQLITE3_MEMORY_BUDGET 10*MB

#ifdef _WIN32
  #include "windows.h"
  #include "commdlg.h"
  #include "winsqlite3.h"
  #include "glcorearb.h"
  #include "wglext.h"
  #include "GL/gl.h"
  #include "hidusage.h"
  #include "win32_struct.c"
  #include "win32_time.c"
  #include "win32_platform.c"
  #include "win32_opengl.c"
  #include "win32_imgui.c"
  #include "win32_main.c"
#endif 

