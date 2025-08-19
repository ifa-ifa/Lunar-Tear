# libreplicant


## Types

-   `StblFile`: Represents a file (a single database).
-   `StblTable`: A single named table within the database.
-   `StblRow`: A `std::vector` of `StblField`s.
-   `StblField`: A single cell in a table. It has a `type` and a `data` member. The format is designed so that the same field in different rows can be of different types, but iv never seen this in practice.
-   `StblFieldData`: A `std::variant` that can hold an `int32_t`, `float`, `std::string`, `std::monostate` (empty).
-   `StblSpatialEntity`: An entry in the special Spatial Entities block, containing name, position, and volume data. Not entirely sure what this data is for. I think it holds cooridnates for enemy spawn locations.

Note that this library will not produce 1-to-1 replicas. the original file has per table string pools, this library produces a single string poo. I havn't noticed this to be an issue.

##  Example

The example demonstrates loading, accessing a specific field, and saving the modified file.

```cpp
#include <iostream>
#include <vector>
#include "libstbl/stbl.h"

int main() {
    StblFile stbl;
    if (!stbl.loadFromFile("A_SOUTH_FIELD_01.settbll")) {
        std::cout << "Failed to load" << std::endl;
        return 1;
    }

    // Access tables
    std::vector<StblTable>& tables = stbl.getTables();
    if (tables.empty()) {
        std::cout << "File has no tables" << "\n";
        return 0;
    }

    // Get a specific table (e.g., the first one).
    StblTable& firstTable = tables[0];
    std::cout << "First table name: " << firstTable.getName() << "\n";

    // Access a specific field (row 0, field 1).
    if (firstTable.getRowCount() > 0 && firstTable.getFieldCount() > 1) {
        StblField& field = firstTable.getRow(0)[1];

        // To modify a field, simply call the appropriate setter.
        // The library automatically synchronizes the internal data and type.
        field.setInt(9001); // This sets both the value and the type.

        // You can safely read data using the optional-based getters.
        if (auto value = field.getInt()) {
            std::cout << "New value is: " << *value << std::endl;
        }
    }

    // Save the changes to a new file.
    if (!stbl.saveToFile("A_SOUTH_FIELD_01_modified.settbll")) {
        std::cout << "Failed to save file" << "\n";
        return 1;
    }

    std::cout << "File modified and saved successfully" << "\n";
    return 0;
}
```

## API

### `StblFile`
-   `bool loadFromFile(path)`: Loads file from disk.
-   `bool saveToFile(path)`: Saves the current data to a new STBL file.
-   `std::vector<StblTable>& getTables()`: Gets a reference to the vector of all tables.
-   `std::vector<StblSpatialEntity>& getSpatialEntities()`: Gets a reference to the vector of all spatial entities.

### `StblTable`
-   `StblRow& getRow(index)`: Gets a reference to a specific row.
-   `const std::string& getName()`: Gets the table's name.
-   `size_t getRowCount()`: Gets number of rows in a table.

### `StblField`
#### Getters
-   `StblFieldType getType() const`: Gets the field's current type (`INT`, `FLOAT`, `STRING`, etc.).
-   `std::optional<int32_t> getInt() const`: Safely gets the value if the type is `INT`.
-   `std::optional<float> getFloat() const`: Safely gets the value if the type is `FLOAT`.
-   `std::optional<std::string> getString() const`: Safely gets the value if the type is `STRING`.
-   `const StblFieldData& getData() const`: Gets the internal `std::variant` for advanced use with `std::visit`.

#### Setters
-   `void setInt(int32_t value)`: Sets the field's type to `INT` and updates its value.
-   `void setFloat(float value)`: Sets the field's type to `FLOAT` and updates its value.
-   `void setString(const std::string& value)`: Sets the field's type to `STRING` and updates its value.
-   `void setDefault()`: Clears the field to its default (empty) state.

## Future Improvements

- Proper SELECT WHERE queries
- Table joins


## Developer Notes

See documentation/stlb.md