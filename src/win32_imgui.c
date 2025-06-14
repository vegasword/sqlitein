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
  [SQLITEIN_ERROR_SQLITE] = "SQLite error",
  [SQLITEIN_ERROR_CANT_OPEN_DATABASE] = "Can't open database",
};

void InitImGui(Win32Context *context)
{
  ImGui imgui = { .ctx = igCreateContext(NULL), .io = igGetIO() };
  imgui.io->IniFilename = NULL;
  imgui.io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  imgui.io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  
  ImGui_ImplWin32_InitForOpenGL(context->window);
  ImGui_ImplOpenGL3_Init(NULL);
  igStyleColorsDark(NULL);
}

static bool dockSpaceInitialized = false;

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
  
  ImGuiID dockSpaceId = igGetID_Str("DockSpace");
  igDockSpaceOverViewport(dockSpaceId, igGetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode, NULL);
  if (!dockSpaceInitialized)
  {
    ImGuiID dockSpaceCentralNodeId = igDockBuilderGetCentralNode(dockSpaceId)->ID;
    ImGuiID tablesNodeId, tableViewerNodeId;
    
    igDockBuilderSplitNode(dockSpaceCentralNodeId, ImGuiDir_Left, 0.2f, &tablesNodeId, &tableViewerNodeId);
    
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
        sqlitein->error = SQLITEIN_NO_ERROR;
        
        char *databasePath = "";
        
        i32 fileOpenResult = Win32_OpenFileDialog(win32, databasePath);
        if (fileOpenResult != 1)
        {
          if (fileOpenResult > 1) sqlitein->error = SQLITEIN_ERROR_CANT_OPEN_DATABASE;
          goto end_menu;
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
          sqlite3_finalize(statement);
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
          sqlite3_finalize(statement);
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
            sqlite3_finalize(statement);
            goto end_menu;
          }
          
          u32 length = sqlite3_column_bytes(statement, 0);
          tables[i].name = (char *)Alloc(arena, length);
          memcpy(tables[i].name, sqlite3_column_text(statement, 0), length);            
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
        igText("%s: %s", sqliteinErrorsMessages[error], sqlite3_errstr(sqlite3_extended_errcode(sqlitein->database.handle)));
      }
      else
      {
        igText("%s", sqliteinErrorsMessages[error]);
      }
    }
    
    igEndMainMenuBar();
  }
        
  // igEnd();
  
  if (errorOccured) igPopStyleColor(1);  
  
  if (igBegin("Tables", NULL, 0))
  {
    SQLiteinTable *tables = database->tables;
    
    if (tables)
    {
      u32 tablesCount = database->tablesCount;
      //TODO: More table metadata
      igText("Count: %d", tablesCount);

      igSeparator();
      
      sqlite3_stmt *statement;
      for (u32 tableIndex = 0; tableIndex < tablesCount; ++tableIndex)
      {
        SQLiteinTable *table = &tables[tableIndex];
        if (igSelectable_Bool(table->name, false, ImGuiSelectableFlags_None, (v2){0}))
        {
          if (sqlitein->currentTableArena.cur > sqlitein->projectArena.cur)
          {
            TmpEnd(&sqlitein->currentTableArena);
          }
          
          TmpBegin(&sqlitein->currentTableArena, arena);
          
          u32 queryLength = (u32)strlen(table->name) + 22;
          char *query = (char *)Alloc(arena, queryLength);
          sprintf_s(query, queryLength, "SELECT COUNT(*) FROM %s", table->name);
          
          TmpEnd(&sqlitein->currentTableArena);
          
          if (sqlite3_prepare_v2(database->handle, query, -1, &statement, 0) != SQLITE_OK)
          {
            sqlitein->error = SQLITEIN_ERROR_SQLITE;
            sqlite3_finalize(statement);
            break;
          }
                      
          if (sqlite3_step(statement) == SQLITE_ERROR)
          {
            sqlitein->error = SQLITEIN_ERROR_SQLITE;
            sqlite3_finalize(statement);
            break;
          }
      
          u32 rowsCount = table->rowsCount = sqlite3_column_int(statement, 0);
          
          queryLength = (u32)strlen(table->name) + 15;
          query = (char *)Alloc(arena, queryLength);
          sprintf_s(query, queryLength, "SELECT * FROM %s", table->name);
          
          TmpEnd(&sqlitein->currentTableArena);
          
          if (sqlite3_prepare_v2(database->handle, query, -1, &statement, 0) != SQLITE_OK)
          {
            sqlitein->error = SQLITEIN_ERROR_SQLITE;
          }
          
          u32 columnsCount = table->columnsCount = sqlite3_column_count(statement);
          table->columnsName = (char **)Alloc(arena, columnsCount * sizeof(char *));

          for (u32 columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
          {
            const char *name = sqlite3_column_name(statement, columnIndex);
            u32 length = (u32)strlen(name);
            table->columnsName[columnIndex] = Alloc(arena, length + 1);
            memcpy(table->columnsName[columnIndex], name, length);
          }

          table->columns = (SQLiteinColumn *)Alloc(arena, rowsCount * columnsCount * sizeof(SQLiteinColumn));
          
          for (u32 rowIndex = 0; rowIndex < rowsCount; ++rowIndex)
          {
            if (sqlite3_step(statement) == SQLITE_ERROR)
            {
              sqlitein->error = SQLITEIN_ERROR_SQLITE;
              sqlite3_finalize(statement);
              break;
            }
            
            for (u32 columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
            {
              SQLiteinColumn *column = &table->columns[rowIndex * columnsCount + columnIndex];
              column->type = sqlite3_column_type(statement, columnIndex);

              switch (column->type)
              {
                case SQLITE_INTEGER: {
                  column->data.integer = sqlite3_column_int64(statement, columnIndex);
                } break;
                  
                case SQLITE_FLOAT: {
                  column->data.real = sqlite3_column_double(statement, columnIndex);
                } break;
                  
                case SQLITE_TEXT: {
                  const uc *text = sqlite3_column_text(statement, columnIndex);
                  i32 length = sqlite3_column_bytes(statement, columnIndex);
                  column->data.text = (char *)Alloc(arena, length + 1);
                  memcpy(column->data.text, text, length);
                } break;
                  
                case SQLITE_BLOB: {
                  const void *blob = sqlite3_column_blob(statement, columnIndex);
                  i32 size = sqlite3_column_bytes(statement, columnIndex);
                  column->data.blob = Alloc(arena, size + 1);
                  memcpy(column->data.blob, blob, size);
                } break;
                
                default: break;
              }
            }
          }
          
          sqlitein->currentTable = table;
          break;
        }
      }        
    }
    igEnd();
  }

  if (igBegin("Current table", NULL, 0))
  {
    SQLiteinTable *table = sqlitein->currentTable;

    if (table && igBeginTable("##", table->columnsCount, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, (v2){0}, 0))
    {
      u32 rowsCount = table->rowsCount;
      u32 columnsCount = table->columnsCount;
      
      igTableNextRow(ImGuiTableRowFlags_Headers, 0);
      
      for (u32 columnIndex = 0; columnIndex < columnsCount; ++columnIndex) 
      {
        igTableSetColumnIndex(columnIndex);
        igNextColumn();
        igText("%s", table->columnsName[columnIndex]);
      }
      
      for (u32 rowIndex = 0; rowIndex < rowsCount; ++rowIndex)
      {
        igTableNextRow(0, 0);

        for (u32 columnIndex = 0; columnIndex < columnsCount; ++columnIndex) 
        {
          igTableSetColumnIndex(columnIndex);
          igNextColumn();
          
          SQLiteinColumn *column = &table->columns[rowIndex * columnsCount + columnIndex];

          switch (column->type)
          {
            case SQLITE_INTEGER: igText("%d", column->data.integer);     break;
            case SQLITE_FLOAT:   igText("%f", column->data.real);        break;
            case SQLITE_TEXT:    igText("%s", column->data.text);        break;
            case SQLITE_BLOB:    igText("BLOB (%p)", column->data.blob); break;
            default:             igText("NULL");                         break;
          }
        }
      }

      igEndTable();
    }
  }
  
  igEnd();
}

void RenderImGui(void)
{
  igRender();
  ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
}
