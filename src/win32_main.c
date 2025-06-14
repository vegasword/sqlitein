#define SQLITE_CHECK(db, result, value, message) if (result != value) Log("%s: %s\n", message, sqlite3_errstr(sqlite3_extended_errcode(db)));

i32 WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
  hInstPrev; cmdline; cmdshow; // To avoid stupid MSVC warning
  
  Arena arena;
  Init(&arena, VirtualAlloc(NULL, 2*GB, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE), 2*GB);
  {
    void *sqlite3Buffer = Alloc(&arena, SQLITE3_MEMORY_BUDGET);
    sqlite3_config(SQLITE_CONFIG_HEAP, sqlite3Buffer, SQLITE3_MEMORY_BUDGET, DEFAULT_ALIGNMENT);
  }
      
  SQLitein *sqlitein = New(&arena, SQLitein);
  
  Win32Context *win32 = (Win32Context *)Alloc(&arena, sizeof(Win32Context));
  InitDebugConsole();
  win32->window = CreateOpenGLContext(hInst, win32);
  InitImGui(win32);
  
  ImGuiData imguiData = (ImGuiData) {
    .arena = &arena,
    .win32 = win32,
    .sqlitein = sqlitein
  };
  
  DEVMODE devMode = (DEVMODE) { .dmSize = sizeof(DEVMODE) };
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
  f32 desiredDelay = 1 / (f32)devMode.dmDisplayFrequency;
  
  PerfCounter deltaCounter = InitPerfCounter();
  StartPerfCounter(&deltaCounter);

  TmpBegin(&sqlitein->projectArena, &arena);
  
  for (;;)
  {
    f32 deltaTime = GetDeltaTime(&deltaCounter);
    imguiData.frameDelay = deltaTime - desiredDelay;
    
    MSG message;
    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
    {      
      if (!win32->quitting)
      {
        TranslateMessage(&message);
        DispatchMessage(&message);
      }
      else
      {
        sqlite3_close(sqlitein->database.handle);
        Win32_Quit(win32);
        return 0;
      }
    }
    
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
    UpdateImGui(&arena, &imguiData);
    RenderImGui();
    
    SwapBuffers(win32->dc);
  }
}

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ImGuiWindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
  ImGui_ImplWin32_WndProcHandler(window, msg, wParam, lParam);
  
  Win32Context *context;
  if (msg == WM_CREATE)
  {
    CREATESTRUCT *createStruct = (CREATESTRUCT *)lParam;
    context = (Win32Context *)createStruct->lpCreateParams;
    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)context);
  }
  else
  {
    context = (Win32Context *)GetWindowLongPtr(window, GWLP_USERDATA);
  }
  
  switch (msg)
  {
    case WM_CLOSE:
    case WM_QUIT:
    case WM_DESTROY: {
      Win32_Quit(context);
    } break;
    
    default: return DefWindowProc(window, msg, wParam, lParam);
  }
  
  return 0;
}
