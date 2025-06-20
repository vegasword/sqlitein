i32 WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
  hInstPrev; cmdline; cmdshow; // To avoid stupid MSVC warning
  
  Arena arena;
  Init(&arena, VirtualAlloc(NULL, 2*GB, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE), 2*GB);
      
  SQLitein *sqlitein = New(&arena, SQLitein);
  
  Win32Context *win32 = (Win32Context *)Alloc(&arena, sizeof(Win32Context));
#if DEBUG
  Win32_InitDebugConsole();
#endif

  win32->window = CreateOpenGLContext(hInst, win32);
  
  MyImGuiContext myImguiContext = (MyImGuiContext) {
    .arena = &arena,
    .win32 = win32,
    .sqlitein = sqlitein
  };
  
  InitImGui(&myImguiContext);
  
  DEVMODE devMode = (DEVMODE) { .dmSize = sizeof(DEVMODE) };
  EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
  f32 desiredDelay = 1 / (f32)devMode.dmDisplayFrequency; //TODO: Should be updatd when changing display device
  
  PerfCounter deltaCounter = InitPerfCounter();
  StartPerfCounter(&deltaCounter);

  TmpBegin(&sqlitein->projectArena, &arena);
  
  bool idling = true;
  f32 chronoBeforeIdling = 0;
  for (;;)
  {
    f32 deltaTime = UpdateDeltaTime(&deltaCounter);
    myImguiContext.frameDelay = deltaTime - desiredDelay;
    
    MSG msg;
    bool input = false;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if (!win32->quitting)
      {
        if (msg.message == WM_MOUSEMOVE || msg.message == WM_MBUTTONDOWN || msg.message == WM_MOUSEWHEEL || msg.message == WM_KEYDOWN)
        {
          input = true;
          idling = false;
          chronoBeforeIdling = 0;
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      else
      {
        sqlite3_close(sqlitein->database.handle);
        Win32_Quit(win32);
        return 0;
      }
    }
    
    if (idling)
    {
      Sleep(60);
    }
    else if (!input && !idling)
    {
      chronoBeforeIdling += deltaTime;
      if (chronoBeforeIdling > IDLE_MODE_TIMER_MS)
      {
        idling = true;
      }
    }
    
    RECT clientRect;
    GetClientRect(win32->window, &clientRect);
    if (clientRect.right == 0 || clientRect.bottom == 0)
    {
      SwapBuffers(win32->dc);
      continue;
    }
    
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
    UpdateImGui(&arena, &myImguiContext);
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
