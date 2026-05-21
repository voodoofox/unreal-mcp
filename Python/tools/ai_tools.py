"""
AI & Behavior Tree Tools for Unreal MCP.

This module provides tools for inspecting Behavior Trees, Blackboards,
and MetaSound assets.
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


def register_ai_tools(mcp: FastMCP):
    """Register AI/BehaviorTree tools with the MCP server."""
    from connection_holder import async_tool, get_unreal_connection

    @async_tool(mcp)
    def list_behavior_trees(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List all Behavior Tree assets under a content browser path.

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
    if ad.asset_class_path.asset_name == 'BehaviorTree':
        results.append({{"name": str(ad.asset_name), "path": str(ad.package_name)}})
print(json.dumps(results))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_behavior_tree_info(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Get full Behavior Tree structure - nodes, decorators, services, blackboard keys.
        Uses native C++ for deep tree traversal.

        Args:
            asset_path: Full asset path to the BehaviorTree
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        response = unreal.send_command("get_behavior_tree_info", {
            "asset_path": asset_path
        })
        if not response:
            return {"success": False, "message": "No response from Unreal Engine"}
        return response

    @async_tool(mcp)
    def list_blackboard_assets(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List all Blackboard Data assets under a content browser path.

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
    if ad.asset_class_path.asset_name == 'BlackboardData':
        results.append({{"name": str(ad.asset_name), "path": str(ad.package_name)}})
print(json.dumps(results))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def list_metasound_assets(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List all MetaSound source assets under a content browser path.

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
    cn = str(ad.asset_class_path.asset_name)
    if 'MetaSound' in cn:
        results.append({{"name": str(ad.asset_name), "path": str(ad.package_name), "class": cn}})
print(json.dumps(results))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_metasound_info(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Get MetaSound asset info - inputs, outputs, and basic properties.

        Args:
            asset_path: Full asset path to the MetaSound asset
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = f"""
import unreal, json
asset = unreal.EditorAssetLibrary.load_asset('{asset_path}')
if asset is None:
    print(json.dumps({{"error": "Asset not found"}}))
else:
    info = {{"name": asset.get_name(), "path": asset.get_path_name(), "class": asset.get_class().get_name()}}
    for prop in ["duration", "is_one_shot", "output_format"]:
        try:
            val = asset.get_editor_property(prop)
            info[prop] = str(val) if not isinstance(val, (bool, int, float, type(None))) else val
        except Exception:
            pass
    print(json.dumps(info))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def list_niagara_data_channels(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List all Niagara Data Channel assets under a content browser path.

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
    if ad.asset_class_path.asset_name == 'NiagaraDataChannelAsset':
        results.append({{"name": str(ad.asset_name), "path": str(ad.package_name)}})
print(json.dumps(results))
"""
        return _run_python(unreal, code)

    logger.info("AI/BehaviorTree tools registered successfully")
