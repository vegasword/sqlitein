LRESULT CALLBACK ImGuiWindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam);

#define PFNGL(name) PFNGL##name##PROC
#define PFNWGL(name) PFNWGL##name##PROC

#if DEBUG
#define GL_FUNC(X) X(PFNGL(DEBUGMESSAGECALLBACK),      glDebugMessageCallback      )

#define X(type, name) static type name;
GL_FUNC(X)
#undef X

#define GL_DEBUG_CASE(buffer, category, subcategory) \
  case GL_DEBUG_##category##_##subcategory##: \
    strncpy_s(buffer, 32, #subcategory, sizeof(#subcategory) - 1); \
    break
    
#pragma warning(push)
#pragma warning(disable: 4100)
void APIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *user)
{
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
  
  char sourceStr[32];
  switch (source)
  {
    GL_DEBUG_CASE(sourceStr, SOURCE, API);
    GL_DEBUG_CASE(sourceStr, SOURCE, WINDOW_SYSTEM);
    GL_DEBUG_CASE(sourceStr, SOURCE, SHADER_COMPILER);
    GL_DEBUG_CASE(sourceStr, SOURCE, THIRD_PARTY);
    GL_DEBUG_CASE(sourceStr, SOURCE, APPLICATION);
    GL_DEBUG_CASE(sourceStr, SOURCE, OTHER);
  }

  char typeStr[32];
  switch (type)
  {
    GL_DEBUG_CASE(typeStr, TYPE, ERROR);
    GL_DEBUG_CASE(typeStr, TYPE, DEPRECATED_BEHAVIOR);
    GL_DEBUG_CASE(typeStr, TYPE, UNDEFINED_BEHAVIOR);
    GL_DEBUG_CASE(typeStr, TYPE, PORTABILITY);
    GL_DEBUG_CASE(typeStr, TYPE, PERFORMANCE);
    GL_DEBUG_CASE(typeStr, TYPE, OTHER);
    default: break;
  }

  char severityStr[32];
  switch (severity)
  {
    GL_DEBUG_CASE(severityStr, SEVERITY, LOW);
    GL_DEBUG_CASE(severityStr, SEVERITY, MEDIUM);
    GL_DEBUG_CASE(severityStr, SEVERITY, HIGH);
  }
  Log("[OPENGL] [%s/%s/%s] %s\n", sourceStr, typeStr, severityStr, message);
}
#pragma warning(pop)
#endif

static PFNWGL(SWAPINTERVALEXT) wglSwapIntervalEXT = NULL;
static PFNWGL(CHOOSEPIXELFORMATARB) wglChoosePixelFormatARB = NULL;
static PFNWGL(CREATECONTEXTATTRIBSARB) wglCreateContextAttribsARB = NULL;

void UpdateViewportDimensions(Win32Context *context)
{
  RECT rect = {0};
  if (GetClientRect(context->window, &rect) != 0)
  {
    context->windowWidth  = rect.right - rect.left;
    context->windowHeight = rect.bottom - rect.top;
    glViewport(0, 0, context->windowWidth, context->windowHeight);
  }  
}

void GetWglFunctions(void)
{ 
  HWND dummy = CreateWindowExA(0, "STATIC", "DummyWindow", WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
  assert(dummy != NULL);

  HDC dc = GetDC(dummy);
  assert(dc != NULL);

  PIXELFORMATDESCRIPTOR desc =
  {
    .nSize      = sizeof(desc),
    .nVersion   = 1,
    .dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    .iPixelType = PFD_TYPE_RGBA,
    .cColorBits = 24
  };

  i32 format = ChoosePixelFormat(dc, &desc);
  assert(format);

  i32 result = DescribePixelFormat(dc, format, sizeof(desc), &desc);
  assert(result);

  result = SetPixelFormat(dc, format, &desc);
  assert(result);

  HGLRC rc = wglCreateContext(dc);
  assert(rc);

  result = wglMakeCurrent(dc, rc);
  assert(result);

  PFNWGL(GETEXTENSIONSSTRINGARB) wglGetExtensionsStringARB = (PFNWGL(GETEXTENSIONSSTRINGARB))wglGetProcAddress("wglGetExtensionsStringARB");
  assert(wglGetExtensionsStringARB);

  const char *extsARB = wglGetExtensionsStringARB(dc);
  assert(extsARB != NULL);

  size_t extsLen = strlen(extsARB);
  
  if (FastStrCmp("WGL_EXT_swap_control", extsARB, extsLen))
  {
    wglSwapIntervalEXT = (PFNWGL(SWAPINTERVALEXT))wglGetProcAddress("wglSwapIntervalEXT");
  }
  
  if (FastStrCmp("WGL_ARB_pixel_format", extsARB, extsLen))
  {
    wglChoosePixelFormatARB = (PFNWGL(CHOOSEPIXELFORMATARB))wglGetProcAddress("wglChoosePixelFormatARB");
  }
  
  if (FastStrCmp("WGL_ARB_create_context", extsARB, extsLen))
  {
    wglCreateContextAttribsARB = (PFNWGL(CREATECONTEXTATTRIBSARB))wglGetProcAddress("wglCreateContextAttribsARB");
  }

  assert(wglChoosePixelFormatARB && wglCreateContextAttribsARB && wglSwapIntervalEXT);

  wglMakeCurrent(NULL, NULL);
  wglDeleteContext(rc);
  ReleaseDC(dummy, dc);
  DestroyWindow(dummy);
}

HWND CreateOpenGLContext(HINSTANCE instance, Win32Context *context)
{
  WNDCLASS windowClass = (WNDCLASS) {
    .lpfnWndProc = ImGuiWindowProc,
    .hInstance = instance,
    .lpszClassName = WINDOW_TITLE"Class",
    .hCursor = LoadCursor(NULL, IDC_ARROW)
  };
  i32 result = RegisterClass(&windowClass);
  assert(result);

  GetWglFunctions();
  
  HWND window = CreateWindowEx(0, WINDOW_TITLE"Class", WINDOW_TITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, (LPVOID)context);
  assert(window);
  context->window = window;
  
  HDC dc = GetDC(window);
  assert(dc);
  context->dc = dc;
  
  i32 attribs[] =
  {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
    WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
    WGL_COLOR_BITS_ARB,     24,
    WGL_DEPTH_BITS_ARB,     24,
    WGL_STENCIL_BITS_ARB,   8,
    WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
    WGL_SAMPLE_BUFFERS_ARB, 1,
    WGL_SAMPLES_ARB,        4,
    0
  };
  
  i32 format; u32 formats;
  result = wglChoosePixelFormatARB(dc, attribs, 0, 1, &format, &formats);
  assert(result && formats);

  PIXELFORMATDESCRIPTOR desc = { .nSize = sizeof(desc) };
  result = DescribePixelFormat(dc, format, desc.nSize, &desc);
  assert(result);

  format = SetPixelFormat(dc, format, &desc);
  assert(format);
  
  i32 contextAttribs[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    WGL_CONTEXT_MINOR_VERSION_ARB, 6,
    WGL_CONTEXT_PROFILE_MASK_ARB,
    WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#if DEBUG
    WGL_CONTEXT_FLAGS_ARB,
    WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
    0
  };
  
  HGLRC glrc = wglCreateContextAttribsARB(dc, 0, contextAttribs);
  assert(glrc);
  context->glrc = glrc;
  
  result = wglMakeCurrent(dc, glrc);
  assert(result);

#define X(type, name) \
  name = (type)wglGetProcAddress(#name); \
  assert(name);
  GL_FUNC(X)
#undef X
  
#if DEBUG
  glDebugMessageCallback(&glDebugCallback, context);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
  wglSwapIntervalEXT(1);
  
  ShowWindow(window, SW_MAXIMIZE);
  
  UpdateViewportDimensions(context);
  
  char title[128] = WINDOW_TITLE;
  strncat_s(title, 128, " - ", _TRUNCATE);
  strncat_s(title, 128, (const char *)glGetString(GL_VERSION), _TRUNCATE);
  SetWindowText(window, title);
  
  return window;
}
