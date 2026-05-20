"""
Audio Tools for Unreal MCP.

This module provides tools for inspecting and managing audio assets
(SoundCue, SoundWave, SoundAttenuation, SoundClass, SoundMix).
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


def register_audio_tools(mcp: FastMCP):
    """Register audio tools with the MCP server."""
    from connection_holder import async_tool, get_unreal_connection

    @async_tool(mcp)
    def list_audio_assets(
        ctx: Context,
        path: str = "/Game",
        asset_type: str = "SoundWave",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List audio assets in the content browser.

        Args:
            path: Content browser path to search
            asset_type: Type to filter  - SoundWave, SoundCue, MetaSoundSource, SoundAttenuation, SoundClass, SoundMix
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
    def get_sound_wave_info(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Get detailed info about a SoundWave asset.

        Args:
            asset_path: Full asset path (e.g. /Game/Audio/MySound)

        Returns:
            Duration, sample rate, channels, compression, looping status.
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = f"""
import unreal, json
asset = unreal.EditorAssetLibrary.load_asset('{asset_path}')
if asset is None:
    print(json.dumps({{"error": "Asset not found"}}))
elif not isinstance(asset, unreal.SoundWave):
    print(json.dumps({{"error": "Asset is not a SoundWave, got " + asset.get_class().get_name()}}))
else:
    info = {{"name": asset.get_name(), "path": asset.get_path_name(), "duration": asset.duration}}
    for prop in ["sample_rate", "num_channels", "looping", "streaming", "sound_group"]:
        try:
            val = asset.get_editor_property(prop)
            info[prop] = str(val) if not isinstance(val, (bool, int, float, type(None))) else val
        except Exception:
            pass
    print(json.dumps(info))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_sound_cue_info(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Get info about a SoundCue asset  - referenced waves, attenuation, class.

        Args:
            asset_path: Full asset path to the SoundCue
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = f"""
import unreal, json
asset = unreal.EditorAssetLibrary.load_asset('{asset_path}')
if asset is None:
    print(json.dumps({{"error": "Asset not found"}}))
elif not isinstance(asset, unreal.SoundCue):
    print(json.dumps({{"error": "Asset is not a SoundCue, got " + asset.get_class().get_name()}}))
else:
    info = {{
        "name": asset.get_name(),
        "path": asset.get_path_name(),
        "duration": asset.duration,
        "max_distance": asset.get_editor_property("max_distance") if hasattr(asset, 'max_distance') else None,
        "volume_multiplier": asset.get_editor_property("volume_multiplier"),
        "pitch_multiplier": asset.get_editor_property("pitch_multiplier"),
    }}
    att = asset.get_editor_property("attenuation_settings")
    if att:
        info["attenuation"] = att.get_path_name()
    sc = asset.get_editor_property("sound_class_object")
    if sc:
        info["sound_class"] = sc.get_path_name()
    print(json.dumps(info))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_sound_attenuation_info(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Get SoundAttenuation settings  - falloff, distance ranges, spatialization.

        Args:
            asset_path: Full asset path to the SoundAttenuation
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
    att = asset.get_editor_property("attenuation") if hasattr(asset, 'attenuation') else asset
    info = {{"name": asset.get_name(), "path": asset.get_path_name(), "class": asset.get_class().get_name()}}
    if hasattr(att, 'falloff_distance'):
        info["falloff_distance"] = att.falloff_distance
    if hasattr(att, 'attenuation_shape'):
        info["shape"] = str(att.attenuation_shape)
    if hasattr(att, 'inner_radius'):
        info["inner_radius"] = att.inner_radius
    if hasattr(att, 'falloff_mode'):
        info["falloff_mode"] = str(att.falloff_mode)
    if hasattr(att, 'b_spatialize'):
        info["spatialize"] = att.b_spatialize
    print(json.dumps(info))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def get_actor_audio_components(
        ctx: Context,
        actor_name: str
    ) -> Dict[str, Any]:
        """
        Get audio components on a level actor  - what sounds are attached and their settings.

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
    comps = target.get_components_by_class(unreal.AudioComponent)
    result = {{"actor": target.get_name(), "audio_components": []}}
    for c in comps:
        entry = {{"name": c.get_name()}}
        snd = c.get_editor_property("sound")
        if snd:
            entry["sound"] = snd.get_path_name()
            entry["sound_class"] = snd.get_class().get_name()
        entry["volume"] = c.get_editor_property("volume_multiplier")
        entry["pitch"] = c.get_editor_property("pitch_multiplier")
        entry["auto_activate"] = c.get_editor_property("auto_activate")
        result["audio_components"].append(entry)
    print(json.dumps(result))
"""
        return _run_python(unreal, code)

    @async_tool(mcp)
    def play_sound_at_location(
        ctx: Context,
        sound_path: str,
        location: list = [0.0, 0.0, 0.0],
        volume: float = 1.0,
        pitch: float = 1.0
    ) -> Dict[str, Any]:
        """
        Play a sound at a world location in the editor (preview).

        Args:
            sound_path: Path to SoundWave or SoundCue asset
            location: [X, Y, Z] world position
            volume: Volume multiplier (0.0 to 1.0)
            pitch: Pitch multiplier
        """
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        code = f"""
import unreal, json
snd = unreal.EditorAssetLibrary.load_asset('{sound_path}')
if snd is None:
    print(json.dumps({{"error": "Sound asset not found"}}))
else:
    loc = unreal.Vector({location[0]}, {location[1]}, {location[2]})
    unreal.GameplayStatics.play_sound_at_location(unreal.EditorLevelLibrary.get_editor_world(), snd, loc, volume_multiplier={volume}, pitch_multiplier={pitch})
    print(json.dumps({{"success": True, "sound": snd.get_name(), "location": [{location[0]}, {location[1]}, {location[2]}]}}))
"""
        return _run_python(unreal, code)

    logger.info("Audio tools registered successfully")
