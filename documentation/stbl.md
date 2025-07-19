# STBL

The STBL (Symbol Table) file format is a data storage format. It functions like a database, containing named tables of data.

There are multiple STBL resoureces. one that is always loaded into memory seems to be `STBL_Controller_GlobalNPC` (+48921f0) which stores some data about all NPCS in the game. another is `STBL_Controller_LocalNPC` which stores different data about the currently loaded level.

## File Layout

An STBL file loaded into memory is a single contiguous block divided into sections in the following order:

1.  File Header: A header at the beginning of the file containing metadata, section offsets, and table counts.
2.  Spatial Entity Block: A data block immediately following the header, containing an array of `STBL_SpatialEntity` structs. 
3.  Table-Specific Data Blocks: For each table defined in the file there is a dedicated data block located at the offset specified in its descriptor. Each block is an array of rows, where each row is a fixed size and consists of 8 byte `STBL_FIELD` entries
4.  Table Descriptor Block: An array of `STBL_TableDescriptor` structs. This is a index for all the tables in the file.
5.  String Pool: A large block at the end of the each table containing all null-terminated strings referenced by the data table.

The order in files straight from the game is: header -> spatial entities -> descriptors -> each table (which has its own unique string pool for itself immediately after) But it wont matter as long as the offsets are correct. My parser uses a single string pool at the end of the file and it seems to work fine.



### `STBL_Controller`

This structure is stored in static memory and is somewhat similar to a header, but external.

```c


struct STBL_Controller {
    struct STBL_FileHeader *header;
    INT64 spatialEntityCount;
    struct STBL_SpatialEntity *spatialEntities;
    INT64 unknown_0;
    struct STBL_TableDescriptor *table_descriptors;
    INT64 table_count;
    struct STBL_TableDescriptor *table_descriptors_copy;
    void *unknown_1;
    INT64 unknown_2;
    void *unknown_3;
    INT64 unknown_4;
    void *unknown_5;
};

```

Use of STBL resoureces is usually done through a static controller rather than a pointer to the file header on the heap.

### `STBL_FileHeader`

```c
struct STBL_FileHeader {
    char magic_bytes[4]; /* "STBL" */
    int version;
    byte padding_0[4];
    int spatialEntityCount;
    int header_size;
    byte padding_1[4];
    int table_descriptor_offset; /* from start of header */
    int table_count;
    byte unknown[48];
};
```


### `STBL_SpatialEntity`

```c
struct STBL_SpatialEntity {

    struct Vector4 position;
    struct Vector4 volume_data;
    char name[32];
    int32_t unknown_params[4];
    byte padding[16];
};
```

### `STBL_Field`

```c
struct STBL_Field {
    enum STBL_FieldType field0_0x0;
    union STBL_FieldData data; 
};
```


### `STBL_FieldType` (enum)

```c

How the data should be used. not to be confused with field *data* type

enum STBL_FieldType {
    // The field has no override. Use a hardcoded default value.
    FIELD_TYPE_DEFAULT = 0,

    FIELD_TYPE_INT = 1,

    FIELD_TYPE_FLOAT = 2, 

    FIELD_TYPE_STRING_OFFSET=3,
    FIELD_TYPE_UNKNOWN=4,
    FIELD_TYPE_ERROR=5
};
```

### `STBL_FieldData`

```c
union STBL_FieldData {
    float as_float;
    int32_t as_int;
    int32_t offset_to_string; // offset from start of header
    byte rawdata[4];
};
```


### `STBL_TableDescriptor`

```c
struct STBL_TableDescriptor {
    char    tableName[32];
    int32_t rowCount;
    int32_t rowSizeInFields;
    int32_t dataOffset; // Offset from the start of the header to the data.
    byte    unknown[20];
};
```


## Game API

```c

float STBL_GetFloatData(STBL_Controller *controller,int tableIndex,int rowIndex,int fieldIndex) // +42a7d0

int STBL_GetIntData(STBL_Controller *controller,int tableIndex,int rowIndex,int fieldIndex) // +42a820

char * STBL_GetStringData(STBL_Controller *controller,int table_index,int rowIndex,int fieldIndex ) // +42a950

```

Returns the data part of the specified field, except strings, where the data in the field is an offset to a string. this offset is resolved and a pointer to that string is returned. `0` or `nullptr` on error.


```c
STBL_FieldType STBL_GetFieldType(STBL_Controller *controller,int tableIndex,int rowIndex,int fieldIndex) // +42a770
```

Returns the FieldType at the specified field. returns `FIELD_TYPE_DEFAULT` (`0`) on error.

```c
int STBL_FindTableIndexByName(STBL_Controller *STBL_controller,char *tableName) // +42a9c0
```

Returns the index of the table with the specified name in the file. `-1` on error. 

```c
int STBL_GetRowCount(STBL_Controller *controller,int tableIndex) // +42a870
```
Returns the amount of rows in the specified table in the file. `0` on error.

```c

int STBL_FindJoinedRowIndices( // +4250f0
    STBL_Controller *primaryController, // Controller for the "left" table of the join
    int searchId,                       // The value for the WHERE clause (e.g., 101)
    char *primaryTableName,             // The "left" table name (e.g., "enemy_group")
    char *secondaryTableName,           // The "right" table name (e.g., "enemy_base")
    int primarySearchColumnIndex,       // Index of the column for the WHERE clause (e.g., column 0 for 'group_id')
    int primaryForeignKeyColumnIndex,   // Index of the column in the left table used for the JOIN (e.g., column 1 for 'enemy_base_name')
    int secondaryKeyColumnIndex,        // Index of the column in the right table used for the JOIN (e.g., column 0 for 'base_name')
    int *out_primaryRowIndex,           // OUT: The row index of the found row in the primary table
    int *out_secondaryRowIndex          // OUT: The row index of the found row in the secondary table
)
```

Performs a join on two tables (can be in different files). Returns as soon as the first match found. Returns 1 on success, 0 on failure. Populates `out_primaryRowIndex` and `out_secondaryRowIndex` with the results. Case insensitive.


```c
bool STBL_InitialiseController(STBL_Controller *pController,STBL_FileHeader *pFile)
```

Initialises a controller from a given file header. returns false (0) on failure, true (1) on success


```c
int STBL_GetSpatialEntityIndex(STBL_Controller *param_1,char *name)```

gets index of spatial entity based on name. returns -1 on error or not found.
