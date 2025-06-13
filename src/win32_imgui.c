typedef struct  {
  ImGuiContext *ctx;
  ImGuiIO *io;
} ImGui;

typedef struct {  
  Arena *arena;
  Win32Context *win32;
  SQLitein *sqlitein;
  f32 frameDelay;
} ImGuiData;

static const char *sqliteinErrorsMessages[] = {
  [SQLITEIN_ERROR_SQLITE] = "SQLite internal error",
  [SQLITEIN_ERROR_CANT_OPEN_DATABASE] = "Can't open database",
};

void InitImGui(HWND window) //TODO: Build docked (https://github.com/ocornut/imgui/issues/4430)
{
  ImGui imgui = (ImGui) {
    .ctx = igCreateContext(NULL),
    .io = igGetIO()
  };
  
  imgui.io->IniFilename = NULL;
  imgui.io->ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
  
  ImGui_ImplWin32_InitForOpenGL(window);
  ImGui_ImplOpenGL3_Init(NULL);
  igStyleColorsDark(NULL);
}

void UpdateImGui(Arena *arena, ImGuiData *data)
{
  Win32Context *win32 = data->win32;
  SQLitein *sqlitein = data->sqlitein;
  SQLiteinDB *database = &sqlitein->database;
  SQLiteinErrors error = sqlitein->error;
  bool errorOccured = error != SQLITEIN_NO_ERROR;
  
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplWin32_NewFrame();
  igNewFrame();
  
  if (igIsKeyDown_Nil(ImGuiKey_Escape))
  {
    win32->quitting = true;
  }
  
  
  if (errorOccured) igPushStyleColor_Vec4(ImGuiCol_MenuBarBg, (v4){255,0,0,255});
  
  if (igBegin("#", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_DockNodeHost))
  {    
    igSetWindowPos_Vec2((ImVec2){0, 0}, ImGuiCond_Once);
    igSetWindowSize_Vec2((ImVec2){(f32)win32->windowWidth, (f32)win32->windowHeight}, ImGuiCond_Always);
        
    if (igBeginMenuBar())
    {
      if (igBeginMenu("File", true))
      {
        if (igMenuItem_Bool("Open", NULL, false, true))
        {
          sqlitein->error = SQLITEIN_NO_ERROR;
          
          char *databasePath = "";
          if (Win32_OpenFileDialog(win32, databasePath) != 0)
          {
            sqlitein->error = SQLITEIN_ERROR_CANT_OPEN_DATABASE;
          }
          
          if (sqlite3_open(databasePath, &database->handle) != SQLITE_OK)
          {
            sqlitein->error = SQLITEIN_ERROR_SQLITE;
            goto end_menu;
          }
          
          sqlite3_stmt *statement;
          if (sqlite3_prepare_v2(database->handle, "SELECT COUNT(*) FROM sqlite_master WHERE type='table'", -1, &statement, 0) != SQLITE_OK)
          {
            sqlitein->error = SQLITEIN_ERROR_SQLITE;
            goto end_menu;
          }
          
          if (sqlite3_step(statement) == SQLITE_ERROR)
          {
            sqlitein->error = SQLITEIN_ERROR_SQLITE;
          }
          
          u32 tablesCount = database->tablesCount = sqlite3_column_int(statement, 0);
            
          if (sqlite3_prepare_v2(database->handle, "SELECT name FROM sqlite_master WHERE type='table'", -1, &statement, 0) != SQLITE_OK)
          {
            sqlitein->error = SQLITEIN_ERROR_SQLITE;
            goto end_menu;
          }
          
          TmpEnd(&sqlitein->projectArena);
          TmpBegin(&sqlitein->projectArena, arena);
          
          database->tables = (SQLiteinTable *)Alloc(arena, tablesCount * sizeof(SQLiteinTable));
          
          SQLiteinTable *tables = database->tables;
          for (u32 i = 0; i < tablesCount; ++i)
          {
            if (sqlite3_step(statement) == SQLITE_ERROR)
            {
              sqlitein->error = SQLITEIN_ERROR_SQLITE;
              goto end_menu;
            }
            
            String *tableName = &tables[i].name;
            tableName->length = sqlite3_column_bytes(statement, 0);
            tableName->data = (char *)Alloc(arena, tableName->length);
            tableName->data = (char *)sqlite3_column_text(statement, 0);
            
            Log("%s\n", tableName->data);
          }
          
          sqlite3_finalize(statement);
        }

end_menu:
        igEndMenu();
      }
    
      if (igBeginMenu("About", true))
      {
        igText("© Alexandre Perché (@vegasword), 2025. All right reserved.");
        igEndMenu();
      }

      
      if (errorOccured)
      {
        if (error == SQLITEIN_ERROR_SQLITE)
        {
          igText("%s (%s)", sqliteinErrorsMessages[error], sqlite3_errstr(sqlite3_extended_errcode(sqlitein->database.handle)));
        }
        else
        {
          igText("%s", sqliteinErrorsMessages[error]);
        }
      }
      
      igEndMenuBar();
    }
    
    if (errorOccured) igPopStyleColor(1);
  }
  
  igEnd();
}

void RenderImGui(void)
{
  igRender();
  ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
}
