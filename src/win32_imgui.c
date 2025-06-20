typedef struct  {
  ImGuiContext *ctx;
  ImGuiIO *io;
} ImGui;

typedef struct {  
  f32 frameDelay;
  Arena *arena;
  Win32Context *win32;
  SQLitein *sqlitein;
  ImFont *font;
} MyImGuiContext;

static const char *sqliteinErrorsMessages[] = {
  [SQLITEIN_ERROR_SQLITE] = "SQLite error",
  [SQLITEIN_ERROR_CANT_OPEN_DATABASE] = "Can't open database",
  [SQLITEIN_ERROR_UNSUPPORTED_VALUE] = "BLOB and NULL values are unsupported for now",
};

void InitImGui(MyImGuiContext *context)
{
  ImGui imgui = { .ctx = igCreateContext(NULL), .io = igGetIO() };
  imgui.io->IniFilename = NULL;
  imgui.io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  imgui.io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImFontAtlas *atlas = imgui.io->Fonts;
  File fontFile = Win32_ReadWholeFile(context->arena, "NotoSans-Regular.ttf");
  context->font = ImFontAtlas_AddFontFromMemoryTTF(atlas, (void *)fontFile.data, fontFile.size, 16, NULL, ImFontAtlas_GetGlyphRangesDefault(atlas)); 
  
  ImGui_ImplWin32_InitForOpenGL(context->win32->window);
  ImGui_ImplOpenGL3_Init(NULL);
  igStyleColorsDark(NULL);  
}

static bool dockSpaceInitialized = false;

void UpdateImGui(Arena *arena, MyImGuiContext *context)
{
  Win32Context *win32 = context->win32;
  SQLitein *sqlitein = context->sqlitein;
  SQLiteinDB *database = &sqlitein->database;
  SQLiteinErrors error = sqlitein->error;
  bool errorOccured = error != SQLITEIN_NO_ERROR;
  
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplWin32_NewFrame();
  igNewFrame();

  igPushFont(context->font);
  
  if (igIsKeyDown_Nil(ImGuiKey_Escape))
  {
    win32->quitting = true;
  }  
  
  ImGuiID dockSpaceId = igGetID_Str("DockSpace");
  igDockSpaceOverViewport(dockSpaceId, igGetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode, NULL);
  if (!dockSpaceInitialized)
  {
    ImGuiID dockSpaceCentralNodeId = igDockBuilderGetCentralNode(dockSpaceId)->ID;
    ImGuiID tablesNodeId, tableViewerNodeId;
    
    igDockBuilderSplitNode(dockSpaceCentralNodeId, ImGuiDir_Left, 0.1f, &tablesNodeId, &tableViewerNodeId);
    
    igDockBuilderDockWindow("Tables", tablesNodeId);
    igDockBuilderDockWindow("Current table", tableViewerNodeId);
    igDockBuilderFinish(dockSpaceCentralNodeId);
    
    dockSpaceInitialized = true;
  }
    
  if (errorOccured) igPushStyleColor_Vec4(ImGuiCol_MenuBarBg, (v4){255,0,0,255});

  if (igBeginMainMenuBar())
  {
    if (igBeginMenu("File", true))
    {
      if (igMenuItem_Bool("Open", "CTRL+O", false, true))
      {
        char *databasePath = "";
        i32 result = Win32_OpenFileDialog(win32, databasePath);
        if (result)
        {
          SQLitein_LoadTables(arena, sqlitein, databasePath);
        }
        else if (result > 1)
        {
          sqlitein->error = SQLITEIN_ERROR_CANT_OPEN_DATABASE;
        }        
      }
      igEndMenu();
    }
  
    if (igBeginMenu("About", true))
    {
      igText("© Alexandre Perché (@vegasword), 2025. All right reserved.");
      igEndMenu();
    }

    
#if DEBUG
    igText("%.2f ms", context->frameDelay);
#endif

    if (errorOccured)
    {
      if (error == SQLITEIN_ERROR_SQLITE)
      {
        igText("%s: %s", sqliteinErrorsMessages[error], sqlite3_errstr(sqlite3_extended_errcode(sqlitein->database.handle)));
      }
      else
      {
        igText("%s", sqliteinErrorsMessages[error]);
      }
    }
    
    igEndMainMenuBar();
  }
  
  if (errorOccured) igPopStyleColor(1);
  
  igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (v2){0});
  
  if (igBegin("Tables", NULL, 0))
  {
    SQLiteinTable *tables = database->tables;
    
    if (tables)
    {
      u32 tablesCount = database->tablesCount;
      //TODO: More table metadata
      igText("Count: %d", tablesCount);

      igSeparator();
      
      ImGuiListClipper *clipper = ImGuiListClipper_ImGuiListClipper();
      ImGuiListClipper_Begin(clipper, tablesCount, 0);

      while (ImGuiListClipper_Step(clipper))
      {        
        for (i32 tableIndex = clipper->DisplayStart; tableIndex < clipper->DisplayEnd; ++tableIndex)
        {
          SQLiteinTable *table = &tables[tableIndex];
        
          if (igSelectable_Bool(table->name, false, ImGuiSelectableFlags_None, (v2){0}) && sqlitein->currentTable != table)
          {            
            SQLitein_LoadTable(arena, sqlitein, table);
          }
        }  
      }
    }
    igEnd();
  }

  if (igBegin("Current table", NULL, 0))
  {
    SQLiteinTable *table = sqlitein->currentTable;
    
    if (table)
    {
      u32 rowsCount = table->rowsCount;
      u32 columnsCount = table->columnsCount;
      
      igPushStyleVar_Vec2(ImGuiStyleVar_FramePadding, (v2){0});
      igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (v2){0});
      igPushStyleVar_Vec2(ImGuiStyleVar_ItemInnerSpacing, (v2){0});
      igPushStyleVar_Vec2(ImGuiStyleVar_CellPadding, (v2){0});
      
      if (igBeginTable("##Table view", table->columnsCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_HighlightHoveredColumn, (v2){0}, 0))
      {
        igTableSetupScrollFreeze(0, 1);
        
        for (u32 columnIndex = 0; columnIndex < columnsCount; ++columnIndex) 
        {
          igTableSetupColumn(table->columnsNames[columnIndex], ImGuiTableColumnFlags_WidthStretch, 0, igGetID_Str("##Table view"));
        }
        
        igTableHeadersRow();
        
        ImGuiListClipper *clipper = ImGuiListClipper_ImGuiListClipper();
        ImGuiListClipper_Begin(clipper, rowsCount, 0);
        
        while (ImGuiListClipper_Step(clipper))
        {
          for (i32 rowIndex = clipper->DisplayStart; rowIndex < clipper->DisplayEnd; ++rowIndex)
          {
            igTableNextRow(0, 0);
            
            for (u32 columnIndex = 0; columnIndex < columnsCount; ++columnIndex) 
            {
              igTableSetColumnIndex(columnIndex);
              igNextColumn();
              
              SQLiteinColumn *column = &table->columns[rowIndex * columnsCount + columnIndex];

              igPushID_Ptr(column);
              
              v2 contentRegionAvailable;
              igGetContentRegionAvail(&contentRegionAvailable);
              igSetNextItemWidth(contentRegionAvailable.x);

              char buffer[KB] = {0};
              memcpy(buffer, column->value, strlen(column->value));
              
              if (sqlitein->nextRowIndex == rowIndex)
              {
                sqlitein->nextRowIndex = -1;
                igSetKeyboardFocusHere(0);
              }

              i32 columnType = table->columnsTypes[columnIndex];
              
              ImGuiInputTextFlags decimalInput = columnType == SQLITE_INTEGER || columnType == SQLITE_FLOAT ? ImGuiInputTextFlags_CharsDecimal : 0;
              if (igInputText("##", buffer, KB, ImGuiInputTextFlags_EnterReturnsTrue | decimalInput, NULL, NULL))
              {
                if (igIsKeyPressed_Bool(ImGuiKey_Enter, false))
                {
                  sqlitein->nextRowIndex = rowIndex + 1;
                  SQLitein_UpdateColumn(arena, sqlitein, rowIndex, columnIndex, columnType, (char *)&buffer);
                }
              }
              
              igPopID();
            }
          }
        }
        
        
        ImGuiListClipper_End(clipper);
        igPopStyleVar(4);
        igEndTable();
      }
    }
    igEnd();
  }
  
  igPopStyleVar(1);
  igPopFont();
}

void RenderImGui(void)
{
  igRender();
  ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
}
