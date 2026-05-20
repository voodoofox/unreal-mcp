# Material & Inspection Tools

Tools for inspecting assets, editing materials, and managing Nanite settings.

## Asset Inspection

| Tool | Description |
|------|-------------|
| `get_blueprint_components` | Get all components of a Blueprint with types, transforms, and mesh assignments |
| `get_blueprint_defaults` | Get Blueprint class default property values (optional filter prefix) |
| `list_assets` | List assets in the content browser by path and type |
| `get_asset_details` | Get detailed info about any asset |
| `get_component_properties` | Get properties of a specific component on a Blueprint |
| `get_blueprint_graph` | Get the node graph of a Blueprint (events, functions, connections) |
| `get_node_pins` | Get input/output pins of a specific Blueprint node |

## Static Mesh & Nanite

| Tool | Description |
|------|-------------|
| `get_static_mesh_info` | Get static mesh info (triangles, vertices, LODs, materials) |
| `list_nanite_status` | List Nanite enabled/disabled status across static meshes |
| `set_static_mesh_nanite_enabled` | Enable or disable Nanite on a static mesh |

## Material Editing

| Tool | Description |
|------|-------------|
| `get_material_info` | Get material info including parent, parameter values |
| `get_material_graph` | Get the full material expression graph |
| `set_material_instance_parent` | Set the parent of a Material Instance |
| `add_material_expression` | Add a material expression node |
| `delete_material_expression` | Remove a material expression node |
| `connect_material_expressions` | Wire two material expression nodes together |
| `connect_material_property` | Connect an expression to a material property (BaseColor, Normal, etc.) |
| `disconnect_material_property` | Disconnect a material property input |
| `set_material_expression_property` | Set a property on a material expression |
| `compile_material` | Recompile a material after edits |

## Skeleton

| Tool | Description |
|------|-------------|
| `get_skeleton_bones` | Get full bone hierarchy with names, parents, and ref pose (C++ native) |
