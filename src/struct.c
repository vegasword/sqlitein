typedef struct {
  char *value;
} SQLiteinColumn;

typedef struct {
  u32 rowsCount;
  u32 columnsCount;
  char *name;
  char **columnsNames;
  i32 *columnsTypes;
  SQLiteinColumn *columns;
} SQLiteinTable;

typedef struct {
  u32 tablesCount;
  sqlite3 *handle;
  SQLiteinTable *tables;
} SQLiteinDB;

typedef struct {
  i32 nextRowIndex;
  i32 nextColumnIndex;
  TmpArena projectArena;
  TmpArena currentViewArena;
  SQLiteinTable *currentTable;
  SQLiteinDB database;
  SQLiteinErrors error;
} SQLitein;
