# Lunar Tear - SETTBLLEditor

This tool allows you to open, view, and edit STBL (`.settbll`) files 

### Viewing Data
-   The color of a cell indicates its type
    -   Light Blue: Integer 
    -   Light Green: Float 
    -   Light Pink: String
    -   Gray/White: Default (empty) or other.
-   Hide Empty Columns: Check this box to hide all the columns that only contain `[DEFAULT]` values.

### Editing Data


Double-click a cell to edit its value. The editor will enforce the cell's existing data type.
To change a cell's data type, you must use the context menu. If the current value cannot be converted, the change will be rejected.

## Future Improvements

- Creating new files from scratch
- Editing spatial data
- Visually display table joins
- undo/redo
- Settings for oclour coding
- Tabs to view multiple files at once
- Remember last open file
- Make it prettier


## Developer Notes

Application was tested with QT 6.9.0. 