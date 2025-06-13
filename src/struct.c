typedef struct {
  char *data;
  u32 length;
} String;

typedef struct {
  String name;
  u32 rowsCount;
  u32 columnsCount;
  //TODO: SQLiteinColumn
} SQLiteinTable;

typedef struct {
  sqlite3 *handle;
  u32 tablesCount;
  SQLiteinTable *tables;
} SQLiteinDB;

typedef struct {
  TmpArena projectArena;
  SQLiteinDB database;
  SQLiteinErrors error;
} SQLitein;

