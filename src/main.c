#define WINDOW_TITLE "SQLitein"
#define IDLE_MODE_TIMER_MS 5000

#include "stdlib.h"
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

#define SQLITE_ENABLE_MEMSYS5
#ifdef _WIN32
  #include "winsqlite3.h"
#endif

#include "type.c"
#include "arena.c"
#include "enum.c"
#include "struct.c"
#include "debug.h"
#include "algorithm.c"
#include "sqlitein.c"

#ifdef _WIN32
  #include "windows.h"
  #include "commdlg.h"
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

