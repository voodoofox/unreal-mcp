"""
Animation Tools for Unreal MCP.

This module provides tools for inspecting animation assets, skeletal meshes,
and controlling animation playback. All commands execute via run_python_in_unreal.
"""

import json
import logging
from typing import Dict, Any, List
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


def register_animation_tools(mcp: FastMCP):
    """Register animation tools with the MCP server."""
    from connection_holder import async_tool, get_unreal_connection

    @async_tool(mcp)
    def list_animation_assets(
        ctx: Context,
        path: str = "/Game",
        asset_type: str = "AnimSequence",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List animation assets in the content browser.

        Args:
            path: Content browser path to search
            asset_type: Type to filter  - AnimSequence, AnimMontage, AnimBlueprint, BlendSpace, BlendSpace1D
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
    if ad.asset_class_path.asset_name == '{asset_type}':
        results.append({{"name": str(ad.asset_name), "path": str(ad.package_name)}})
print(json.dumps(results))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_animation_info(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Get detailed info about an AnimSequence or AnimMontage asset.

        Args:
            asset_path: Full asset path (e.g. /Game/Animations/Walk)

        Returns:
            Duration, frame count, rate scale, skeleton, and type-specific info.
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
    info = {{"name": asset.get_name(), "class": asset.get_class().get_name()}}
    if isinstance(asset, (unreal.AnimSequence, unreal.AnimMontage)):
        info["duration"] = asset.get_play_length()
        if hasattr(asset, 'rate_scale'):
            info["rate_scale"] = asset.rate_scale
        try:
            info["rate_scale"] = asset.get_editor_property("rate_scale")
        except Exception:
            pass
        skel = asset.get_editor_property("skeleton")
        if skel:
            info["skeleton"] = skel.get_path_name()
    if isinstance(asset, unreal.AnimSequence):
        for attr in ["number_of_frames", "number_of_keys", "number_of_sampled_frames", "number_of_sampled_keys"]:
            if hasattr(asset, attr):
                info[attr] = getattr(asset, attr)
    if isinstance(asset, unreal.AnimMontage):
        sections = []
        for i in range(asset.get_num_sections()):
            sections.append(str(asset.get_section_name(i)))
        info["sections"] = sections
    print(json.dumps(info))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_skeleton_info(
        ctx: Context,
        asset_path: str,
        max_bones: int = 200
    ) -> Dict[str, Any]:
        """
        Get bone hierarchy from a Skeleton, SkeletalMesh, or AnimSequence asset.
        Uses native C++ handler for full bone access including ref pose positions.

        Args:
            asset_path: Path to Skeleton, SkeletalMesh, or animation asset
            max_bones: Max bones to return (default 200)
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        response = unreal.send_command("get_skeleton_bones", {
            "asset_path": asset_path,
            "max_bones": max_bones
        })
        if not response:
            return {"success": False, "message": "No response from Unreal Engine"}
        return response

    @async_tool(mcp)
    def get_blend_space_info(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Get BlendSpace info  - axis settings and sample points.

        Args:
            asset_path: Path to BlendSpace or BlendSpace1D asset
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
    info = {{"name": asset.get_name(), "class": asset.get_class().get_name()}}
    if hasattr(asset, 'axis_to_scale_animation'):
        info["axis_to_scale"] = str(asset.axis_to_scale_animation)
    skel = asset.get_editor_property("skeleton")
    if skel:
        info["skeleton"] = skel.get_path_name()
    info["duration"] = asset.get_play_length() if hasattr(asset, 'get_play_length') else None
    print(json.dumps(info))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_actor_animation_state(
        ctx: Context,
        actor_name: str
    ) -> Dict[str, Any]:
        """
        Get current animation state of a skeletal mesh actor in the level.

        Args:
            actor_name: Name of the actor to inspect
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = f"""
import unreal, json
actors = unreal.EditorLevelLibrary.get_all_level_actors()
target = None
for a in actors:
    if a.get_name() == '{actor_name}' or a.get_actor_label() == '{actor_name}':
        target = a
        break
if target is None:
    print(json.dumps({{"error": "Actor not found"}}))
else:
    info = {{"actor": target.get_name(), "class": target.get_class().get_name()}}
    comps = target.get_components_by_class(unreal.SkeletalMeshComponent)
    if len(comps) > 0:
        smc = comps[0]
        info["skeletal_mesh"] = smc.get_editor_property("skeletal_mesh_asset").get_path_name() if smc.get_editor_property("skeletal_mesh_asset") else None
        info["animation_mode"] = str(smc.get_editor_property("animation_mode"))
        anim_class = smc.get_editor_property("anim_class")
        if anim_class:
            info["anim_blueprint"] = anim_class.get_path_name()
        anim_asset = smc.get_editor_property("animation_data").anim_to_play if hasattr(smc.get_editor_property("animation_data"), 'anim_to_play') else None
        if anim_asset:
            info["playing_animation"] = anim_asset.get_path_name()
    else:
        info["skeletal_mesh_components"] = 0
    print(json.dumps(info))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def set_actor_animation(
        ctx: Context,
        actor_name: str,
        animation_path: str,
        looping: bool = True
    ) -> Dict[str, Any]:
        """
        Set an animation on a skeletal mesh actor (preview in editor).

        Args:
            actor_name: Name of the actor
            animation_path: Path to AnimSequence asset
            looping: Whether the animation should loop
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = f"""
import unreal, json
actors = unreal.EditorLevelLibrary.get_all_level_actors()
target = None
for a in actors:
    if a.get_name() == '{actor_name}' or a.get_actor_label() == '{actor_name}':
        target = a
        break
if target is None:
    print(json.dumps({{"error": "Actor not found"}}))
else:
    comps = target.get_components_by_class(unreal.SkeletalMeshComponent)
    if len(comps) == 0:
        print(json.dumps({{"error": "No SkeletalMeshComponent on actor"}}))
    else:
        smc = comps[0]
        anim = unreal.EditorAssetLibrary.load_asset('{animation_path}')
        if anim is None:
            print(json.dumps({{"error": "Animation asset not found"}}))
        else:
            smc.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)
            smc.set_animation(anim)
            smc.set_editor_property("play_rate", 1.0)
            print(json.dumps({{"success": True, "actor": target.get_name(), "animation": anim.get_name(), "looping": {str(looping).lower()}}}))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_animation_notifies(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Get animation notify events from an AnimSequence or AnimMontage.

        Args:
            asset_path: Path to animation asset
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
    notifies = []
    if hasattr(asset, 'notifies'):
        for n in asset.notifies:
            entry = {{"time": n.get_editor_property("trigger_time_offset"), "notify_name": n.get_editor_property("notify_name").to_string() if hasattr(n, 'notify_name') else str(n.get_class().get_name())}}
            notifies.append(entry)
    info = {{"name": asset.get_name(), "notify_count": len(notifies), "notifies": notifies}}
    print(json.dumps(info))
"""
        return _run_python(unreal, code)

    logger.info("Animation tools registered successfully")
