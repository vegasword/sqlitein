typedef struct {
  i32 type;
  union {
    i64 integer;
    double real;
    char *text;
    void *blob;
  } data;
} SQLiteinColumn;

typedef struct {
  char *name;
  u32 rowsCount;
  u32 columnsCount;
  char **columnsName;
  SQLiteinColumn *columns;
} SQLiteinTable;

typedef struct {
  sqlite3 *handle;
  u32 tablesCount;
  SQLiteinTable *tables;
} SQLiteinDB;

typedef struct {
  TmpArena projectArena;
  TmpArena currentTableArena;
  SQLiteinTable *currentTable;
  SQLiteinDB database;
  SQLiteinErrors error;
} SQLitein;
