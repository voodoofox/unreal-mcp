"""
Level & World Tools for Unreal MCP.

This module provides tools for inspecting levels, world settings,
and level streaming configuration.
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


def register_level_tools(mcp: FastMCP):
    """Register level/world tools with the MCP server."""
    from connection_holder import async_tool, get_unreal_connection

    @async_tool(mcp)
    def get_current_level_info(
        ctx: Context,
    ) -> Dict[str, Any]:
        """
        Get info about the currently open level  - name, path, actor count, world settings.
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = """
import unreal, json
world = unreal.EditorLevelLibrary.get_editor_world()
actors = unreal.EditorLevelLibrary.get_all_level_actors()
actor_classes = {}
for a in actors:
    cls = a.get_class().get_name()
    actor_classes[cls] = actor_classes.get(cls, 0) + 1
info = {
    "level_name": world.get_name() if world else "Unknown",
    "level_path": world.get_path_name() if world else "Unknown",
    "actor_count": len(actors),
    "actor_classes": actor_classes,
}
ws = unreal.GameplayStatics.get_game_mode(world)
if ws:
    info["game_mode"] = ws.get_class().get_name()
print(json.dumps(info))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def list_levels(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List all Level (map) assets under a content browser path.

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
    cname = ad.asset_class_path.asset_name
    if cname == 'World':
        results.append({{"name": str(ad.asset_name), "path": str(ad.package_name)}})
print(json.dumps(results))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_world_settings(
        ctx: Context,
    ) -> Dict[str, Any]:
        """
        Get the current level's WorldSettings  - game mode override, streaming settings, etc.
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = """
import unreal, json
world = unreal.EditorLevelLibrary.get_editor_world()
if world is None:
    print(json.dumps({"error": "No world loaded"}))
else:
    ws = world.get_world_settings()
    info = {"level": world.get_name()}
    if ws:
        gm = ws.get_editor_property("default_game_mode")
        info["default_game_mode"] = gm.get_name() if gm else None
        info["enable_world_bounds_checks"] = ws.get_editor_property("enable_world_bounds_checks") if hasattr(ws, 'enable_world_bounds_checks') else None
        info["kill_z"] = ws.get_editor_property("kill_z") if hasattr(ws, 'kill_z') else None
    print(json.dumps(info))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_level_streaming_info(
        ctx: Context,
    ) -> Dict[str, Any]:
        """
        Get streaming level configuration  - sub-levels, their types, and load status.
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = """
import unreal, json
world = unreal.EditorLevelLibrary.get_editor_world()
if world is None:
    print(json.dumps({"error": "No world loaded"}))
else:
    levels = world.get_streaming_levels()
    result = {"persistent_level": world.get_name(), "streaming_levels": []}
    for sl in levels:
        entry = {
            "package_name": sl.get_world_asset_package_f_name(),
            "class": sl.get_class().get_name(),
            "should_be_loaded": sl.get_editor_property("should_be_loaded") if hasattr(sl, 'should_be_loaded') else None,
            "should_be_visible": sl.get_editor_property("should_be_visible") if hasattr(sl, 'should_be_visible') else None,
        }
        if hasattr(sl, 'level_transform'):
            t = sl.level_transform
            entry["transform"] = {"location": [t.translation.x, t.translation.y, t.translation.z]}
        result["streaming_levels"].append(entry)
    print(json.dumps(result))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def open_level(
        ctx: Context,
        level_path: str
    ) -> Dict[str, Any]:
        """
        Open a level in the editor.

        Args:
            level_path: Content path to the level (e.g. /Game/Maps/MyLevel)
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = f"""
import unreal, json
success = unreal.EditorLevelLibrary.load_level('{level_path}')
print(json.dumps({{"success": success, "level": "{level_path}"}}))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_level_actor_summary(
        ctx: Context,
        class_filter: str = ""
    ) -> Dict[str, Any]:
        """
        Get a summary of actors in the current level, optionally filtered by class.

        Args:
            class_filter: Only include actors of this class (e.g. StaticMeshActor, PointLight). Empty = all classes.
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        filter_line = ""
        if class_filter:
            filter_line = f"if a.get_class().get_name() != '{class_filter}': continue"

        code = f"""
import unreal, json
actors = unreal.EditorLevelLibrary.get_all_level_actors()
results = []
for a in actors:
    {filter_line}
    loc = a.get_actor_location()
    results.append({{
        "name": a.get_name(),
        "label": a.get_actor_label(),
        "class": a.get_class().get_name(),
        "location": [loc.x, loc.y, loc.z],
    }})
print(json.dumps({{"count": len(results), "actors": results[:500]}}))
"""
        return _run_python(unreal, code)

    logger.info("Level tools registered successfully")
