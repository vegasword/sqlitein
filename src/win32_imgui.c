typedef struct  {
  ImGuiContext *ctx;
  ImGuiIO *io;
} ImGui;

typedef struct {  
  Arena *arena;
  Win32Context *win32;
  f32 frameDelay;
} ImGuiDebugData;

void InitImGui(HWND window)
{
  ImGui imgui = {
    .ctx = igCreateContext(NULL),
    .io = igGetIO()
  };
  
  imgui.io->IniFilename = NULL;
  
  imgui.io->ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard
                        | ImGuiConfigFlags_DockingEnable;
  
  ImGui_ImplWin32_InitForOpenGL(window);
  ImGui_ImplOpenGL3_Init(NULL);
  igStyleColorsDark(NULL);
}

void UpdateImGui(Arena *arena, ImGuiDebugData *data)
{
  Win32Context *win32 = data->win32;
  
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplWin32_NewFrame();
  igNewFrame();
  
  if (igIsKeyDown_Nil(ImGuiKey_Escape))
  {
    win32->quitting = true;
  }
  
  
  if (igBegin("Docker (not the Linux virtualizer)", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_DockNodeHost))
  {    
    igSetWindowPos_Vec2((ImVec2){0, 0}, ImGuiCond_Once);
    igSetWindowSize_Vec2((ImVec2){(f32)win32->windowWidth, (f32)win32->windowHeight}, ImGuiCond_Always);
    
    if (igBeginMenuBar())
    {
      if (igBeginMenu("File", true))
      {
        if (igMenuItem_Bool("Open", NULL, false, true))
        {
          Win32_OpenFileDialog(arena, win32);
        }
      
        if (igMenuItem_Bool("Save", NULL, false, true))
        {
        }
      
        igEndMenu();
      }
    
    
      if (igBeginMenu("About", true))
      {
        igText("© Alexandre Perché. All right reserved.");
        igEndMenu();
      }
        
      igEndMenuBar();
    }    
  }
  
  igEnd();
}

void RenderImGui(void)
{
  igRender();
  ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
}
