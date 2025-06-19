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
  
  if (sqlitein->currentViewArena.cur > sqlitein->projectArena.cur)
  {
    TmpEnd(&sqlitein->currentViewArena);
  }
  
  TmpBegin(&sqlitein->currentViewArena, arena);

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

  TmpEnd(&sqlitein->currentViewArena);

  if (sqlite3_prepare_v2(database->handle, query, -1, &statement, 0) != SQLITE_OK)
  {
    sqlitein->error = SQLITEIN_ERROR_SQLITE;
  }

  u32 columnsCount = table->columnsCount = sqlite3_column_count(statement);
  table->columnsNames = (char **)Alloc(arena, columnsCount * sizeof(char *));
  table->columnsTypes = (i32 *)Alloc(arena, columnsCount * sizeof(i32));

  for (u32 columnIndex = 0; columnIndex < columnsCount; ++columnIndex)
  {
    const char *name = sqlite3_column_name(statement, columnIndex);
    u32 length = (u32)strlen(name);
    table->columnsNames[columnIndex] = (char *)Alloc(arena, length + 1);
    memcpy(table->columnsNames[columnIndex], name, length);
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
      i32 columnType = table->columnsTypes[columnIndex] = sqlite3_column_type(statement, columnIndex);

      if (columnType == SQLITE_NULL)
      {
        continue;
      }
  
      char *value = (char *)sqlite3_column_text(statement, columnIndex);
      i32 valueLength = sqlite3_column_bytes(statement, columnIndex) + 1;
      column->value = (char *)Alloc(arena, valueLength);
      memcpy(column->value, value, valueLength);
    }
  }

  sqlitein->currentTable = table;
  return;
}

void SQLitein_UpdateColumn(Arena *arena, SQLitein *sqlitein, i32 rowIndex, i32 columnIndex, i32 columnType, char *newValue)
{
  SQLiteinDB *database = &sqlitein->database;
  SQLiteinTable *table = sqlitein->currentTable;
  char *tableName = table->name;
  u32 tableNameLength = (u32)strlen(tableName);
  char *columnName = table->columnsNames[columnIndex];
  sqlite3_stmt *statement;
  
  sqlitein->error = SQLITEIN_NO_ERROR;
    
  TmpArena tmp = {0};
  TmpBegin(&tmp, arena);
  
  char rowIndexStr[13] = {0};
  _itoa_s(rowIndex, rowIndexStr, 13, 10);
  u32 queryLength =  tableNameLength + (u32)strlen(rowIndexStr) + 52;
  char *query = (char *)Alloc(arena, queryLength);
  sprintf_s(query, queryLength, "SELECT ROWID FROM %s ORDER BY ROWID LIMIT 1 OFFSET %s", tableName, rowIndexStr); //BUG: This doesn't work if two values are the same column of multiples rows

  if (sqlite3_prepare_v2(database->handle, query, -1, &statement, 0) != SQLITE_OK)
  {
    sqlitein->error = SQLITEIN_ERROR_SQLITE;
    sqlite3_finalize(statement);
    return;
  }
  
  i32 sqliteRowId = -1;
  if (sqlite3_step(statement) == SQLITE_ERROR)
  {
    sqlitein->error = SQLITEIN_ERROR_SQLITE;
    sqlite3_finalize(statement);
    return;
  }
  else
  {
    sqliteRowId = sqlite3_column_int(statement, 0);
  }
    
  char sqliteRowIdStr[13] = {0};
  _itoa_s(sqliteRowId, sqliteRowIdStr, 13, 10);
  queryLength = tableNameLength + (u32)strlen(columnName) + (u32)strlen(newValue) + (u32)strlen(sqliteRowIdStr) + 41;
  query = (char *)Alloc(arena, queryLength);

  TmpEnd(&tmp);
    
  switch (columnType)
  {
    case SQLITE_INTEGER: case SQLITE_FLOAT: {
      sprintf_s(query, queryLength, "UPDATE %s SET %s=%s WHERE ROWID=%s", tableName, columnName, newValue, sqliteRowIdStr);
    } break;
    
    case SQLITE_TEXT: {
      sprintf_s(query, queryLength, "UPDATE %s SET %s='%s' WHERE ROWID=%s", tableName, columnName, newValue, sqliteRowIdStr);
    } break;

    //TODO: BLOB and NULL values

    default: {
      sqlitein->error = SQLITEIN_ERROR_UNSUPPORTED_VALUE;
      return;
    };
  }

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
  
  sqlite3_finalize(statement);
  SQLitein_LoadTable(arena, sqlitein, table); // Reloads the table
}
