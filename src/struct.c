typedef struct {
  i32 type;
  char *value;
} SQLiteinColumn;

typedef struct {
  u32 rowsCount;
  u32 columnsCount;
  char *name;
  char **columnsName;
  SQLiteinColumn *columns;
} SQLiteinTable;

typedef struct {
  u32 tablesCount;
  sqlite3 *handle;
  SQLiteinTable *tables;
} SQLiteinDB;

typedef struct {
  TmpArena projectArena;
  TmpArena currentTableArena;
  SQLiteinTable *currentTable;
  SQLiteinDB database;
  SQLiteinErrors error;
} SQLitein;
