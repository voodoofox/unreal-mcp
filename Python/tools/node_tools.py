"""
Blueprint Node Tools for Unreal MCP.

This module provides tools for manipulating Blueprint graph nodes and connections.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_blueprint_node_tools(mcp: FastMCP):
    """Register Blueprint node manipulation tools with the MCP server."""
    from connection_holder import async_tool
    
    @async_tool(mcp)
    def add_blueprint_event_node(
        ctx: Context,
        blueprint_name: str,
        event_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add an event node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            event_name: Name of the event. Use 'Receive' prefix for standard events:
                       - 'ReceiveBeginPlay' for Begin Play
                       - 'ReceiveTick' for Tick
                       - etc.
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from connection_holder import get_unreal_connection
        
        try:
            # Handle default value within the method body
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "event_name": event_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding event node '{event_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_event_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Event node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding event node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @async_tool(mcp)
    def add_blueprint_input_action_node(
        ctx: Context,
        blueprint_name: str,
        action_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add an input action event node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            action_name: Name of the input action to respond to
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from connection_holder import get_unreal_connection
        
        try:
            # Handle default value within the method body
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "action_name": action_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding input action node for '{action_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_input_action_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Input action node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding input action node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @async_tool(mcp)
    def add_blueprint_function_node(
        ctx: Context,
        blueprint_name: str,
        target: str,
        function_name: str,
        params = None,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a function call node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            target: Target object for the function (component name or self)
            function_name: Name of the function to call
            params: Optional parameters to set on the function node
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from connection_holder import get_unreal_connection
        
        try:
            # Handle default values within the method body
            if params is None:
                params = {}
            if node_position is None:
                node_position = [0, 0]
            
            command_params = {
                "blueprint_name": blueprint_name,
                "target": target,
                "function_name": function_name,
                "params": params,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding function node '{function_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_function_node", command_params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Function node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding function node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
            
    @async_tool(mcp)
    def connect_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        source_node_id: str,
        source_pin: str,
        target_node_id: str,
        target_pin: str
    ) -> Dict[str, Any]:
        """
        Connect two nodes in a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            source_node_id: ID of the source node
            source_pin: Name of the output pin on the source node
            target_node_id: ID of the target node
            target_pin: Name of the input pin on the target node
            
        Returns:
            Response indicating success or failure
        """
        from connection_holder import get_unreal_connection
        
        try:
            params = {
                "blueprint_name": blueprint_name,
                "source_node_id": source_node_id,
                "source_pin": source_pin,
                "target_node_id": target_node_id,
                "target_pin": target_pin
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Connecting nodes in blueprint '{blueprint_name}'")
            response = unreal.send_command("connect_blueprint_nodes", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node connection response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error connecting nodes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @async_tool(mcp)
    def add_blueprint_variable(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        variable_type: str,
        is_exposed: bool = False
    ) -> Dict[str, Any]:
        """
        Add a variable to a Blueprint.
        
        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable
            variable_type: Type of the variable (Boolean, Integer, Float, Vector, etc.)
            is_exposed: Whether to expose the variable to the editor
            
        Returns:
            Response indicating success or failure
        """
        from connection_holder import get_unreal_connection
        
        try:
            params = {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "variable_type": variable_type,
                "is_exposed": is_exposed
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding variable '{variable_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_variable", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Variable creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding variable: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @async_tool(mcp)
    def add_blueprint_get_self_component_reference(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a node that gets a reference to a component owned by the current Blueprint.
        This creates a node similar to what you get when dragging a component from the Components panel.
        
        Args:
            blueprint_name: Name of the target Blueprint
            component_name: Name of the component to get a reference to
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from connection_holder import get_unreal_connection
        
        try:
            # Handle None case explicitly in the function
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding self component reference node for '{component_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_get_self_component_reference", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Self component reference node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding self component reference node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @async_tool(mcp)
    def add_blueprint_self_reference(
        ctx: Context,
        blueprint_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a 'Get Self' node to a Blueprint's event graph that returns a reference to this actor.
        
        Args:
            blueprint_name: Name of the target Blueprint
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from connection_holder import get_unreal_connection
        
        try:
            if node_position is None:
                node_position = [0, 0]
                
            params = {
                "blueprint_name": blueprint_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding self reference node to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_self_reference", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Self reference node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding self reference node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @async_tool(mcp)
    def find_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        node_type = None,
        event_type = None
    ) -> Dict[str, Any]:
        """
        Find nodes in a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            node_type: Optional type of node to find (Event, Function, Variable, etc.)
            event_type: Optional specific event type to find (BeginPlay, Tick, etc.)
            
        Returns:
            Response containing array of found node IDs and success status
        """
        from connection_holder import get_unreal_connection
        
        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_type": node_type,
                "event_type": event_type
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Finding nodes in blueprint '{blueprint_name}'")
            response = unreal.send_command("find_blueprint_nodes", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node find response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error finding nodes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @async_tool(mcp)
    def set_blueprint_node_pin_default(ctx: Context, blueprint_name: str, node_id: str, pin_name: str, default_value: str) -> Dict[str, Any]:
        """Set the default value of a pin on a Blueprint node.

        Args:
            ctx: The MCP context
            blueprint_name: Name of the Blueprint
            node_id: GUID of the node (from get_blueprint_graph)
            pin_name: Name of the pin to set
            default_value: The default value as a string (e.g. '5.0', 'true', '(X=1,Y=2,Z=3)')

        Returns:
            Dict with pin_name and default_value on success
        """
        from connection_holder import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("set_blueprint_node_pin_default", {
                "blueprint_name": blueprint_name,
                "node_id": node_id,
                "pin_name": pin_name,
                "default_value": default_value
            })
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ------------------------------------------------------------------
    # Extended graph-building tools: Variable Set/Get, Branch, Cast,
    # Select, Delete, Disconnect. Added to support full Blueprint graphs
    # (control flow, casts, typed select) from MCP without BP-editor work.
    # ------------------------------------------------------------------

    def _send(cmd_name: str, params: dict) -> Dict[str, Any]:
        from connection_holder import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command(cmd_name, params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response
        except Exception as e:
            return {"success": False, "message": str(e)}

    @async_tool(mcp)
    def add_blueprint_variable_set(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        node_position=None,
    ) -> Dict[str, Any]:
        """Add a Variable SET node (K2Node_VariableSet) for an existing variable.

        The variable must already exist on the Blueprint (use add_blueprint_variable
        first). The Set node exposes exec in/out pins and a data input pin named after
        the variable  - connect a value source to that pin.
        """
        params = {"blueprint_name": blueprint_name, "variable_name": variable_name}
        if node_position is not None:
            params["node_position"] = node_position
        return _send("add_blueprint_variable_set", params)

    @async_tool(mcp)
    def add_blueprint_variable_get(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        node_position=None,
    ) -> Dict[str, Any]:
        """Add a Variable GET node (K2Node_VariableGet) for an existing variable.

        Pure node; output pin is named after the variable.
        """
        params = {"blueprint_name": blueprint_name, "variable_name": variable_name}
        if node_position is not None:
            params["node_position"] = node_position
        return _send("add_blueprint_variable_get", params)

    @async_tool(mcp)
    def add_blueprint_branch(
        ctx: Context,
        blueprint_name: str,
        node_position=None,
    ) -> Dict[str, Any]:
        """Add a Branch node (K2Node_IfThenElse). Pins: Condition (bool input),
        Then (exec output true), Else (exec output false)."""
        params = {"blueprint_name": blueprint_name}
        if node_position is not None:
            params["node_position"] = node_position
        return _send("add_blueprint_branch", params)

    @async_tool(mcp)
    def add_blueprint_cast(
        ctx: Context,
        blueprint_name: str,
        target_class: str,
        pure: bool = False,
        node_position=None,
    ) -> Dict[str, Any]:
        """Add a Cast To X node (K2Node_DynamicCast).

        target_class: bare name ('EndlessRunnerGameMode'), prefixed ('AFoo'),
        or path ('/Script/Module.Class'). pure=True for a pure cast (no exec pins).
        Default impure cast has exec in + Success/Failed exec outputs.

        Pin names: 'Object' (input), 'As<ClassName>' (output data), 'then' (exec success),
        'CastFailed' (exec fail).
        """
        params = {"blueprint_name": blueprint_name, "target_class": target_class, "pure": pure}
        if node_position is not None:
            params["node_position"] = node_position
        return _send("add_blueprint_cast", params)

    @async_tool(mcp)
    def add_blueprint_select(
        ctx: Context,
        blueprint_name: str,
        num_options: int = 2,
        node_position=None,
    ) -> Dict[str, Any]:
        """Add a Select node (K2Node_Select)  - picks one of N typed values by index/bool.

        num_options: how many Option pins to expose (2-32, defaults to 2 = ternary).
        The node's return type auto-adjusts to match the connected Option type.
        Pin names: 'Option 0', 'Option 1', ... 'Index' (selector), 'ReturnValue' (output).
        """
        params = {"blueprint_name": blueprint_name, "num_options": num_options}
        if node_position is not None:
            params["node_position"] = node_position
        return _send("add_blueprint_select", params)

    @async_tool(mcp)
    def delete_blueprint_node(
        ctx: Context,
        blueprint_name: str,
        node_id: str,
    ) -> Dict[str, Any]:
        """Remove a node from the Blueprint's event graph by GUID. Also breaks all
        its pin links. Non-reversible  - use when you mistakenly added a node."""
        return _send("delete_blueprint_node", {
            "blueprint_name": blueprint_name,
            "node_id": node_id,
        })

    @async_tool(mcp)
    def disconnect_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        source_node_id: str,
        source_pin: str,
        target_node_id: str,
        target_pin: str,
    ) -> Dict[str, Any]:
        """Break a single pin-to-pin link between two nodes. Useful when you need to
        surgically remove one connection without disturbing others. NOTE: as of the
        2026-04-16 plugin fix, connect_blueprint_nodes auto-breaks an existing source
        when a new one is connected to the same data input pin  - so this is mainly
        needed for exec pins or rare replacement cases."""
        return _send("disconnect_blueprint_nodes", {
            "blueprint_name": blueprint_name,
            "source_node_id": source_node_id,
            "source_pin": source_pin,
            "target_node_id": target_node_id,
            "target_pin": target_pin,
        })

    logger.info("Blueprint node tools registered successfully")