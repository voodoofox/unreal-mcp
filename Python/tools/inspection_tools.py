"""
Inspection Tools for Unreal MCP.

This module provides tools for inspecting Blueprint contents and browsing Content Browser assets.
"""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_inspection_tools(mcp: FastMCP):
    """Register inspection tools with the MCP server."""
    from connection_holder import async_tool

    @async_tool(mcp)
    def get_blueprint_components(
        ctx: Context,
        blueprint_name: str
    ) -> Dict[str, Any]:
        """Get all components of a Blueprint with their types, transforms, and mesh assignments."""
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_blueprint_components", {
                "blueprint_name": blueprint_name
            })

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get blueprint components response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting blueprint components: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def get_blueprint_defaults(
        ctx: Context,
        blueprint_name: str,
        property_filter: str = ""
    ) -> Dict[str, Any]:
        """Get Blueprint class default property values. Optional filter prefix to narrow results."""
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"blueprint_name": blueprint_name}
            if property_filter:
                params["property_filter"] = property_filter

            response = unreal.send_command("get_blueprint_defaults", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get blueprint defaults response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting blueprint defaults: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def list_assets(
        ctx: Context,
        path: str,
        type: str = "",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """List assets in a Content Browser path. Type filter: StaticMesh, Blueprint, Material, SoundWave, etc."""
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"path": path}
            if type:
                params["type"] = type
            params["recursive"] = recursive

            response = unreal.send_command("list_assets", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"List assets response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error listing assets: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def get_asset_details(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """Get detailed info about a specific asset including class, size, and dependencies."""
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_asset_details", {
                "asset_path": asset_path
            })

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get asset details response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting asset details: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def get_material_info(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """Get material info including parent, scalar/vector/texture parameter values."""
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_material_info", {
                "asset_path": asset_path
            })

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get material info response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting material info: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def get_static_mesh_info(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """Get static mesh geometry info: bounds, LOD vertex/triangle counts, material slots."""
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_static_mesh_info", {
                "asset_path": asset_path
            })

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get static mesh info response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting static mesh info: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def get_component_properties(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        property_filter: str = ""
    ) -> Dict[str, Any]:
        """Get all properties of a specific component in a Blueprint. Optional filter prefix."""
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name
            }
            if property_filter:
                params["property_filter"] = property_filter

            response = unreal.send_command("get_component_properties", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get component properties response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting component properties: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def get_blueprint_graph(
        ctx: Context,
        blueprint_name: str,
        node_type_filter: str = ""
    ) -> Dict[str, Any]:
        """
        Get the full event graph of a Blueprint with all nodes, pins, and connections.

        Args:
            blueprint_name: Name of the Blueprint to inspect
            node_type_filter: Optional filter - only nodes whose class name contains this string
                             (e.g. "Event", "CallFunction", "Variable")

        Returns:
            Dict with nodes array. Each node has: node_id, node_type, node_title, position, pins.
            Each pin has: name, direction, category, is_connected, connected_to.
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"blueprint_name": blueprint_name}
            if node_type_filter:
                params["node_type_filter"] = node_type_filter

            response = unreal.send_command("get_blueprint_graph", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get blueprint graph response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting blueprint graph: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def get_node_pins(
        ctx: Context,
        blueprint_name: str,
        node_id: str
    ) -> Dict[str, Any]:
        """
        Get detailed pin information for a specific node in a Blueprint event graph.

        Args:
            blueprint_name: Name of the Blueprint
            node_id: GUID string of the node (from get_blueprint_graph results)

        Returns:
            Dict with detailed pin info: name, direction, category, sub_category,
            default_value, is_connected, connections.
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_node_pins", {
                "blueprint_name": blueprint_name,
                "node_id": node_id
            })

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get node pins response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting node pins: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def list_nanite_status(
        ctx: Context,
        path: str,
        filter: str = "all",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List static meshes in a Content Browser path with their Nanite enabled/disabled status.

        Args:
            path: Content Browser path (e.g. "/Game/CustomStages")
            filter: "all" (default), "enabled", or "disabled"  - controls which meshes are returned
            recursive: Recurse into subfolders (default True)

        Returns:
            Dict with total_count, nanite_enabled_count, nanite_disabled_count, and meshes array
            (each entry: name, path, nanite_enabled).
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("list_nanite_status", {
                "path": path,
                "filter": filter,
                "recursive": recursive
            })

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"List nanite status response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error listing nanite status: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def set_static_mesh_nanite_enabled(
        ctx: Context,
        asset_path: str,
        enabled: bool = True
    ) -> Dict[str, Any]:
        """
        Enable or disable Nanite on a single static mesh asset. Marks the package dirty;
        you must Save All in the editor afterwards to persist the change.

        Args:
            asset_path: Full asset object path (e.g. "/Game/CustomStages/Foo/Bar.Bar")
            enabled: True to enable Nanite, False to disable (default True)

        Returns:
            Dict with path, was_enabled, now_enabled, changed.
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_static_mesh_nanite_enabled", {
                "asset_path": asset_path,
                "enabled": enabled
            })

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set static mesh nanite enabled response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting nanite enabled: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def get_material_graph(
        ctx: Context,
        asset_path: str,
        resolve_instance: bool = True
    ) -> Dict[str, Any]:
        """
        Get the full material expression graph: all nodes (with stable IDs and type-specific
        data), all connections, and which expression is wired to each top-level material input
        pin (world_position_offset, base_color, normal, roughness, etc.).

        If asset_path is a Material Instance and resolve_instance is True (default), the parent
        chain is walked to the base UMaterial  - Material Instances don't own a graph, only
        parameter overrides.

        Args:
            asset_path: Material or Material Instance object path
                (e.g. "/Game/Materials/M_Track.M_Track")
            resolve_instance: If True, follow instance parent chain to base UMaterial

        Returns:
            Dict with name, path, class, expression_count, expressions[], connections[],
            material_inputs{}, blend_mode, shading_model, use_material_attributes.

            Each expression has: id, type (e.g. "MaterialFunctionCall", "Add",
            "VertexNormalWS"), pos_x, pos_y, plus type-specific fields like function_path,
            parameter_name, default_value, value, text.

            Each connection has: from_id, from_output, to_id, to_input, to_input_name.

            material_inputs is a dict keyed by pin name (world_position_offset, base_color,
            metallic, specular, roughness, anisotropy, emissive_color, opacity, opacity_mask,
            normal, tangent, subsurface_color, clear_coat, clear_coat_roughness,
            ambient_occlusion, refraction, pixel_depth_offset, material_attributes); each
            value is {connected, expression_id?, output_index?}.
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_material_graph", {
                "asset_path": asset_path,
                "resolve_instance": resolve_instance
            })

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get material graph response: {len(str(response))} chars")
            return response

        except Exception as e:
            error_msg = f"Error getting material graph: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def set_material_instance_parent(
        ctx: Context,
        asset_path: str,
        parent_path: str
    ) -> Dict[str, Any]:
        """
        Reparent a Material Instance Constant to a different base material (or instance).
        Marks the instance dirty; you must Save All to persist.

        Args:
            asset_path: Path to the Material Instance Constant (e.g.
                "/Game/Foo/MI_Bar.MI_Bar")
            parent_path: Path to the new parent material or material instance

        Returns:
            Dict with instance, old_parent, new_parent.
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_material_instance_parent", {
                "asset_path": asset_path,
                "parent_path": parent_path
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            logger.info(f"Set material instance parent response: {response}")
            return response
        except Exception as e:
            error_msg = f"Error setting material instance parent: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def add_material_expression(
        ctx: Context,
        material_path: str,
        expression_type: str,
        pos_x: int = 0,
        pos_y: int = 0
    ) -> Dict[str, Any]:
        """
        Create a new material expression node in a Material's graph.

        Args:
            material_path: Path to the base UMaterial (instances are not supported)
            expression_type: Class name with or without "MaterialExpression" prefix.
                Examples: "Add", "Multiply", "Subtract", "Divide", "Constant",
                "Constant3Vector", "VertexNormalWS", "WorldPosition", "ScalarParameter",
                "VectorParameter", "TextureSampleParameter2D", "MaterialFunctionCall",
                "Custom", "Reroute", "NamedRerouteDeclaration".
            pos_x, pos_y: Editor canvas position

        Returns:
            Dict with material, expression_id (use this for connect/set ops),
            type, pos_x, pos_y. The id is the index in the material's expression
            collection  - note that deleting other expressions can shift indices.
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_material_expression", {
                "material_path": material_path,
                "expression_type": expression_type,
                "pos_x": pos_x,
                "pos_y": pos_y
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            logger.info(f"Add material expression response: {response}")
            return response
        except Exception as e:
            error_msg = f"Error adding material expression: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def delete_material_expression(
        ctx: Context,
        material_path: str,
        expression_id: int
    ) -> Dict[str, Any]:
        """
        Delete a material expression by id. Note that remaining expression indices
        may shift after deletion  - re-fetch the graph if you need stable ids for
        further edits.
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("delete_material_expression", {
                "material_path": material_path,
                "expression_id": expression_id
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            logger.info(f"Delete material expression response: {response}")
            return response
        except Exception as e:
            error_msg = f"Error deleting material expression: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def connect_material_expressions(
        ctx: Context,
        material_path: str,
        from_id: int,
        to_id: int,
        from_output_name: str = "",
        to_input_name: str = ""
    ) -> Dict[str, Any]:
        """
        Wire one expression's output to another expression's input. Output/input
        names match the pin names shown in the Material Editor (e.g. "A", "B" for
        Add/Multiply, "" for the default unnamed output).

        Args:
            material_path: Base material path
            from_id: Source expression id (from get_material_graph or add_material_expression)
            to_id: Destination expression id
            from_output_name: Pin name on source. Empty = first/default output.
            to_input_name: Pin name on destination. Empty = first/default input.

        Returns:
            Dict with connected (bool).
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("connect_material_expressions", {
                "material_path": material_path,
                "from_id": from_id,
                "to_id": to_id,
                "from_output_name": from_output_name,
                "to_input_name": to_input_name
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            logger.info(f"Connect material expressions response: {response}")
            return response
        except Exception as e:
            error_msg = f"Error connecting material expressions: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def connect_material_property(
        ctx: Context,
        material_path: str,
        from_id: int,
        property_name: str,
        from_output_name: str = ""
    ) -> Dict[str, Any]:
        """
        Wire a node's output to a top-level material output pin (BaseColor, Normal,
        WorldPositionOffset, etc.).

        Args:
            material_path: Base material path
            from_id: Source expression id
            property_name: One of: base_color, metallic, specular, roughness,
                anisotropy, emissive_color, opacity, opacity_mask, normal, tangent,
                world_position_offset (or "wpo"), subsurface_color, ambient_occlusion
                (or "ao"), refraction, pixel_depth_offset, clear_coat, clear_coat_roughness.
                Underscores optional, case-insensitive.
            from_output_name: Pin name on source. Empty = first output.

        Returns:
            Dict with connected (bool).
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("connect_material_property", {
                "material_path": material_path,
                "from_id": from_id,
                "property_name": property_name,
                "from_output_name": from_output_name
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            logger.info(f"Connect material property response: {response}")
            return response
        except Exception as e:
            error_msg = f"Error connecting material property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def disconnect_material_property(
        ctx: Context,
        material_path: str,
        property_name: str
    ) -> Dict[str, Any]:
        """Disconnect whatever expression is currently wired to a top-level material pin."""
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("disconnect_material_property", {
                "material_path": material_path,
                "property_name": property_name
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            logger.info(f"Disconnect material property response: {response}")
            return response
        except Exception as e:
            error_msg = f"Error disconnecting material property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def set_material_expression_property(
        ctx: Context,
        material_path: str,
        expression_id: int,
        property_name: str,
        property_value: str
    ) -> Dict[str, Any]:
        """
        Set a UPROPERTY on a material expression node via reflection. The value is
        a string that will be parsed by Unreal's ImportText.

        Common patterns:
            * MaterialFunctionCall.MaterialFunction = "/Game/Materials/MF_CurvedWorld.MF_CurvedWorld"
              (raw asset paths are auto-wrapped to Object'...' format)
            * Custom.Code = "return saturate(InVal);"
            * ScalarParameter.ParameterName = "BeatBounce"
            * ScalarParameter.DefaultValue = "1.5"
            * Multiply.ConstA = "1.0"
            * Multiply.ConstB = "1.0"
            * Constant3Vector.Constant = "(R=1.0,G=0.5,B=0.0)"
            * VectorParameter.DefaultValue = "(R=1.0,G=0.5,B=0.0,A=1.0)"

        For MaterialFunctionCall.MaterialFunction the function is auto-refreshed
        (UpdateFromFunctionResource) so the node's input/output pins update.

        Args:
            material_path: Base material path
            expression_id: Target expression id
            property_name: UPROPERTY name (case-sensitive, as declared in the .h)
            property_value: String-encoded value for ImportText
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_material_expression_property", {
                "material_path": material_path,
                "expression_id": expression_id,
                "property_name": property_name,
                "property_value": property_value
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            logger.info(f"Set material expression property response: {response}")
            return response
        except Exception as e:
            error_msg = f"Error setting material expression property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def add_custom_node_input(
        ctx: Context,
        material_path: str,
        expression_id: int,
        input_name: str
    ) -> Dict[str, Any]:
        """
        Append a named input to a Custom HLSL material node (MaterialExpressionCustom),
        preserving its existing inputs AND their wired connections.

        Use this instead of editing `Custom.Inputs` through set_material_expression_property /
        ImportText: that path rebuilds the whole input array and drops existing connections.
        After adding, `input_name` becomes a usable variable inside the node's HLSL `Code`,
        and you can wire an expression into it with connect_material_expressions (the input
        name is the destination pin). Rejects duplicate names and non-Custom expressions.

        Args:
            material_path: Base material path
            expression_id: Target Custom expression id (from get_material_graph)
            input_name: Name of the new input (becomes an HLSL variable in Code)

        Returns:
            Dict with material, expression_id, input_name, input_index, success
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_custom_node_input", {
                "material_path": material_path,
                "expression_id": expression_id,
                "input_name": input_name
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            logger.info(f"Add custom node input response: {response}")
            return response
        except Exception as e:
            error_msg = f"Error adding custom node input: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def compile_material(
        ctx: Context,
        material_path: str
    ) -> Dict[str, Any]:
        """
        Force-recompile a material after editing its graph. Marks the package dirty.
        Save All in the editor afterwards to persist.
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("compile_material", {
                "material_path": material_path
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            logger.info(f"Compile material response: {response}")
            return response
        except Exception as e:
            error_msg = f"Error compiling material: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @async_tool(mcp)
    def get_niagara_deep_inspect(
        ctx: Context,
        system_path: str
    ) -> Dict[str, Any]:
        """
        Deep inspection of a Niagara system - all emitters, modules (with usage/script),
        renderers, simulation stages, event handlers, data interfaces, and user parameters.

        Args:
            system_path: Asset path to the Niagara system
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_niagara_deep_inspect", {
                "system_path": system_path
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            return {"success": False, "message": str(e)}

    @async_tool(mcp)
    def import_niagara_system_spec(
        ctx: Context,
        system_path: str,
        spec: dict
    ) -> Dict[str, Any]:
        """
        Apply a system spec (from export_niagara_system_spec) to configure a Niagara system.
        Uses batch mode for single compile at the end.

        Args:
            system_path: Target system to configure
            spec: The spec dict (or the 'spec' field from export output)
        """
        from connection_holder import get_unreal_connection
        import json as _json

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            commands = []

            for emitter in spec.get("emitters", []):
                ei = emitter.get("index", 0)

                # Sim target
                if emitter.get("sim_target"):
                    commands.append({
                        "type": "set_niagara_emitter_sim_target",
                        "params": {"emitter_index": ei, "sim_target": emitter["sim_target"]}
                    })

                # Scalability
                scal = emitter.get("scalability", {})
                if scal.get("spawn_count_scale") is not None:
                    commands.append({
                        "type": "set_niagara_scalability_override",
                        "params": {"emitter_index": ei, "spawn_count_scale": scal["spawn_count_scale"]}
                    })

                # Curve DI keypoints
                for di in emitter.get("data_interfaces", []):
                    di_name = di.get("name")
                    if not di_name:
                        continue
                    curve_params = {"emitter_index": ei, "di_name": di_name}
                    has_curves = False
                    for key in ["curve", "x_curve", "y_curve", "z_curve",
                                "red_curve", "green_curve", "blue_curve", "alpha_curve"]:
                        if key in di and di[key]:
                            curve_params[key] = di[key]
                            has_curves = True
                    if has_curves:
                        commands.append({
                            "type": "set_niagara_curve_data",
                            "params": curve_params
                        })

                    # DI properties (non-curve)
                    for prop in di.get("properties", []):
                        pname = prop.get("name")
                        pval = prop.get("value")
                        if pname and pval and pname != "Curve":
                            commands.append({
                                "type": "set_niagara_di_property",
                                "params": {
                                    "emitter_index": ei,
                                    "di_name": di_name,
                                    "property_name": pname,
                                    "property_value": str(pval)
                                }
                            })

            if not commands:
                return {"success": True, "message": "No commands generated from spec", "commands": 0}

            # Execute all via batch
            response = unreal.send_command("batch_niagara_commands", {
                "system_path": system_path,
                "commands": commands
            })
            if not response:
                return {"success": False, "message": "No response from batch"}

            result = response.get("result", response)
            return {
                "success": True,
                "commands_sent": len(commands),
                "succeeded": result.get("succeeded", 0),
                "failed": result.get("failed", 0),
            }

        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Inspection tools registered successfully")
