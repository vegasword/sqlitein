void SQLitein_LoadTables(Arena *arena, SQLitein *sqlitein, char *databasePath)
{
  SQLiteinDB *database = &sqlitein->database;
  sqlite3_stmt *statement;
  
  sqlitein->error = SQLITEIN_NO_ERROR;

  if (sqlite3_open(databasePath, &database->handle) != SQLITE_OK)
  {
    sqlitein->error = SQLITEIN_ERROR_SQLITE;
    return;
  }

  if (sqlite3_prepare_v2(database->handle, "SELECT COUNT(*) FROM sqlite_master WHERE type='table'", -1, &statement, 0) != SQLITE_OK)
  {
    sqlitein->error = SQLITEIN_ERROR_SQLITE;
    sqlite3_finalize(statement);
    return;
  }

  if (sqlite3_step(statement) == SQLITE_ERROR)
  {
    sqlitein->error = SQLITEIN_ERROR_SQLITE;
    sqlite3_finalize(statement);
    return;
  }

  u32 tablesCount = database->tablesCount = sqlite3_column_int(statement, 0);
  
  if (sqlite3_prepare_v2(database->handle, "SELECT name FROM sqlite_master WHERE type='table'", -1, &statement, 0) != SQLITE_OK)
  {
    sqlitein->error = SQLITEIN_ERROR_SQLITE;
    sqlite3_finalize(statement);
    return;
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
      return;
    }
  
    u32 length = sqlite3_column_bytes(statement, 0) + 1;
    tables[i].name = (char *)Alloc(arena, length);
    memcpy(tables[i].name, sqlite3_column_text(statement, 0), length);            
  }

  sqlite3_finalize(statement);
}

void SQLitein_LoadTable(Arena *arena, SQLitein *sqlitein, SQLiteinTable *table)
{
  SQLiteinDB *database = &sqlitein->database;
  sqlite3_stmt *statement;
  
  sqlitein->error = SQLITEIN_NO_ERROR;

  TmpBegin(&sqlitein->currentTableArena, arena);

  u32 queryLength = (u32)strlen(table->name) + 22;
  char *query = (char *)Alloc(arena, queryLength);
  sprintf_s(query, queryLength, "SELECT COUNT(*) FROM %s", table->name);

  if (sqlite3_prepare_v2(database->handle, query, -1, &statement, 0) != SQLITE_OK)
  {
    sqlitein->error = SQLITEIN_ERROR_SQLITE;
    sqlite3_finalize(statement);
    return;
  }
          
  if (sqlite3_step(statement) == SQLITE_ERROR)
  {
    sqlitein->error = SQLITEIN_ERROR_SQLITE;
    sqlite3_finalize(statement);
    return;
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
    table->columnsName[columnIndex] = (char *)Alloc(arena, length + 1);
    memcpy(table->columnsName[columnIndex], name, length);
  }

  table->columns = (SQLiteinColumn *)Alloc(arena, rowsCount * columnsCount * sizeof(SQLiteinColumn));

  for (u32 rowIndex = 0; rowIndex < rowsCount; ++rowIndex)
  {
    if (sqlite3_step(statement) == SQLITE_ERROR)
    {
      sqlitein->error = SQLITEIN_ERROR_SQLITE;
      sqlite3_finalize(statement);
      return;
    }

    for (u32 columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
    {
      SQLiteinColumn *column = &table->columns[rowIndex * columnsCount + columnIndex];
      column->type = sqlite3_column_type(statement, columnIndex);
  
      char *value = (char *)sqlite3_column_text(statement, columnIndex);
      i32 valueLength = sqlite3_column_bytes(statement, columnIndex) + 1;
      column->value = (char *)Alloc(arena, valueLength);
      memcpy(column->value, value, valueLength);
    }
  }

  sqlitein->currentTable = table;
  return;
}
