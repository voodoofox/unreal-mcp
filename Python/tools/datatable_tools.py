"""
DataTable Tools for Unreal MCP.

This module provides tools for inspecting and modifying DataTable assets.
All commands execute via run_python_in_unreal.
"""

import json
import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def _run_python(unreal_conn, code: str) -> Dict[str, Any]:
    """Run Python code inside Unreal and return parsed result."""
    response = unreal_conn.send_command("run_python_in_unreal", {"code": code})
    if not response:
        return {"success": False, "message": "No response from Unreal Engine"}
    if response.get("status") == "error":
        return {"success": False, "message": response.get("error", "Unknown error")}
    output = response.get("result", {}).get("output", "").strip()
    if output:
        try:
            return {"success": True, "result": json.loads(output)}
        except json.JSONDecodeError:
            return {"success": True, "result": output}
    return {"success": True, "result": None}


def register_datatable_tools(mcp: FastMCP):
    """Register DataTable tools with the MCP server."""
    from connection_holder import async_tool, get_unreal_connection

    @async_tool(mcp)
    def list_data_tables(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List all DataTable assets under a content browser path.

        Args:
            path: Content browser path to search
            recursive: Search subdirectories
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = f"""
import unreal, json
eal = unreal.EditorAssetLibrary
paths = eal.list_assets('{path}', recursive={recursive}, include_folder=False)
results = []
for p in paths:
    ad = eal.find_asset_data(p)
    if ad.asset_class_path.asset_name == 'DataTable':
        results.append({{"name": str(ad.asset_name), "path": str(ad.package_name)}})
print(json.dumps(results))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_data_table_info(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Get DataTable metadata  - row struct type, row count, column names.

        Args:
            asset_path: Full asset path to the DataTable
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = f"""
import unreal, json
dt = unreal.EditorAssetLibrary.load_asset('{asset_path}')
if dt is None:
    print(json.dumps({{"error": "Asset not found"}}))
elif not isinstance(dt, unreal.DataTable):
    print(json.dumps({{"error": "Asset is not a DataTable, got " + dt.get_class().get_name()}}))
else:
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(dt)
    row_struct = dt.get_editor_property("row_struct")
    info = {{
        "name": dt.get_name(),
        "path": dt.get_path_name(),
        "row_struct": row_struct.get_name() if row_struct else "Unknown",
        "row_count": len(row_names),
        "row_names": [str(n) for n in row_names],
    }}
    print(json.dumps(info))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_data_table_row(
        ctx: Context,
        asset_path: str,
        row_name: str
    ) -> Dict[str, Any]:
        """
        Get a single row from a DataTable by row name.

        Args:
            asset_path: Full asset path to the DataTable
            row_name: Name of the row to retrieve

        Returns:
            Row data as key-value pairs (property names and their values).
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = f"""
import unreal, json
dt = unreal.EditorAssetLibrary.load_asset('{asset_path}')
if dt is None:
    print(json.dumps({{"error": "Asset not found"}}))
else:
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(dt)
    if '{row_name}' not in [str(n) for n in row_names]:
        print(json.dumps({{"error": "Row not found: {row_name}"}}))
    else:
        col_names = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(dt, '')
        result = {{"row_name": "{row_name}", "fields": {{}}}}
        row_struct = dt.get_editor_property("row_struct")
        if row_struct:
            for prop in row_struct.properties():
                prop_name = str(prop.get_name())
                vals = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(dt, prop_name)
                idx = [str(n) for n in row_names].index('{row_name}')
                if idx < len(vals):
                    result["fields"][prop_name] = vals[idx]
        print(json.dumps(result))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_data_table_as_json(
        ctx: Context,
        asset_path: str,
        max_rows: int = 100
    ) -> Dict[str, Any]:
        """
        Export an entire DataTable as JSON (all rows, all columns).

        Args:
            asset_path: Full asset path to the DataTable
            max_rows: Maximum rows to return (default 100, prevents huge responses)
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = f"""
import unreal, json
dt = unreal.EditorAssetLibrary.load_asset('{asset_path}')
if dt is None:
    print(json.dumps({{"error": "Asset not found"}}))
elif not isinstance(dt, unreal.DataTable):
    print(json.dumps({{"error": "Not a DataTable"}}))
else:
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(dt)
    row_struct = dt.get_editor_property("row_struct")
    prop_names = [str(p.get_name()) for p in row_struct.properties()] if row_struct else []
    columns = {{}}
    for pn in prop_names:
        columns[pn] = unreal.DataTableFunctionLibrary.get_data_table_column_as_string(dt, pn)
    rows = []
    for i, rn in enumerate(row_names):
        if i >= {max_rows}:
            break
        row = {{"_row_name": str(rn)}}
        for pn in prop_names:
            row[pn] = columns[pn][i] if i < len(columns[pn]) else None
        rows.append(row)
    result = {{
        "name": dt.get_name(),
        "row_struct": row_struct.get_name() if row_struct else "Unknown",
        "total_rows": len(row_names),
        "returned_rows": len(rows),
        "columns": prop_names,
        "rows": rows,
    }}
    print(json.dumps(result))
"""
        return _run_python(unreal, code)

    logger.info("DataTable tools registered successfully")
