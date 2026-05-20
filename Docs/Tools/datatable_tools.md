# DataTable Tools

Tools for inspecting and exporting DataTable assets.

## list_data_tables

List all DataTable assets under a content browser path.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | string | `/Game` | Content browser path |
| `recursive` | bool | `true` | Search subdirectories |

## get_data_table_info

Get DataTable metadata  - row struct type, row count, row names.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Full asset path to the DataTable |

## get_data_table_row

Get a single row from a DataTable by name.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Full asset path to the DataTable |
| `row_name` | string | Name of the row to retrieve |

Returns all fields as key-value pairs.

## get_data_table_as_json

Export an entire DataTable as JSON (all rows, all columns).

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `asset_path` | string | | Full asset path to the DataTable |
| `max_rows` | int | `100` | Max rows to return (prevents huge responses) |
