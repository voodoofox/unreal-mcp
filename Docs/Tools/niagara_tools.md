# Niagara VFX Tools

71 commands for comprehensive Niagara particle system control, including Custom HLSL module creation. The deepest Niagara MCP integration available.

## System Lifecycle

| Tool | Description |
|------|-------------|
| `create_niagara_system` | Create a new Niagara system asset |
| `get_niagara_overview` | Get system overview (emitters, renderers, sim target) |
| `get_niagara_deep_inspect` | Full system dump - modules, DIs, sim stages, events, user params |
| `export_niagara_system_spec` | Export entire system to JSON (renderers, params, curves, everything) |
| `import_niagara_system_spec` | Apply exported spec to configure a system via batch |
| `set_niagara_system_property` | Set system-level properties |
| `compile_niagara_system` | Recompile system |
| `set_niagara_fixed_bounds` | Set fixed simulation bounds |

## Emitter Management

| Tool | Description |
|------|-------------|
| `add_niagara_emitter` | Add emitter from template or existing asset (39 engine templates available) |
| `set_niagara_emitter_enabled` | Enable/disable emitter |
| `remove_niagara_emitter` | Remove emitter from system |
| `rename_niagara_emitter` | Rename emitter |
| `set_niagara_emitter_property` | Set emitter properties |
| `get_niagara_emitter_properties` | Read emitter properties |
| `set_niagara_emitter_sim_target` | Switch between CPU and GPU simulation |
| `duplicate_niagara_emitter` | Duplicate an emitter |
| `get_niagara_stateless_emitter_info` | Inspect UE 5.7 stateless emitters via reflection |
| `set_niagara_stateless_module_property` | Write properties on stateless emitter modules |

## Renderer Management

| Tool | Description |
|------|-------------|
| `add_niagara_renderer` | Add renderer (Sprite, Ribbon, Mesh, Light) |
| `remove_niagara_renderer` | Remove renderer |
| `set_niagara_renderer_property` | Set renderer properties |
| `get_niagara_renderer_properties` | Read renderer properties |
| `get_niagara_renderer_bindings` | Read renderer attribute bindings |
| `set_niagara_renderer_binding` | Set renderer attribute binding |
| `move_niagara_renderer` | Reorder renderers |

## Module Management

| Tool | Description |
|------|-------------|
| `list_niagara_modules` | List modules on an emitter |
| `add_niagara_module` | Add module by script path |
| `remove_niagara_module` | Remove module |
| `set_niagara_module_enabled` | Enable/disable module |
| `get_niagara_module_inputs` | Read rapid iteration parameters |
| `set_niagara_module_input` | Set rapid iteration parameter value |
| `get_niagara_module_pins` | Read module pins (static switches) |
| `set_niagara_module_pin_value` | Set module pin value |
| `search_niagara_modules` | Search available modules in asset registry |
| `add_niagara_set_parameter` | Add parameter assignment node |
| `set_niagara_module_override` | Set module override value |

## ViewModel (Static Switches + Dynamic Inputs)

| Tool | Description |
|------|-------------|
| `niagara_vm_get_inputs` | List ALL inputs including static switches with values |
| `niagara_vm_set_input` | Set any input value (same code path as editor UI) |
| `niagara_vm_set_dynamic_input` | Set dynamic input expressions (Random Range, etc.) |

## User Parameters

| Tool | Description |
|------|-------------|
| `get_niagara_user_parameters` | Read system-level user parameters |
| `set_niagara_user_parameter` | Set user parameter value |
| `add_niagara_user_parameter` | Add new user parameter |

## Data Interfaces

| Tool | Description |
|------|-------------|
| `get_niagara_di_properties` | Read any DI property via reflection (works on all DI types) |
| `set_niagara_di_property` | Write any DI property via reflection |
| `get_niagara_curve_data` | Read curve keypoints (Float, Vector, Color, Vector2D curves) |
| `set_niagara_curve_data` | Write curve keypoints with interpolation modes |
| `get_niagara_data_channel_info` | Inspect data channel assets (variables, properties) |

## Simulation Stages

| Tool | Description |
|------|-------------|
| `add_niagara_simulation_stage` | Add simulation stage with script/graph initialization |
| `remove_niagara_simulation_stage` | Remove by name |
| `get_niagara_sim_stage_properties` | Read all stage properties via reflection |
| `set_niagara_sim_stage_property` | Write any stage property via reflection |
| `move_niagara_simulation_stage` | Reorder stages |

## Event Handlers

| Tool | Description |
|------|-------------|
| `add_niagara_event_handler` | Add event handler with script/graph initialization |
| `remove_niagara_event_handler` | Remove by usage ID |
| `set_niagara_event_handler_property` | Modify source event, execution mode, max events |

## Scratch Pad / Custom HLSL

| Tool | Description |
|------|-------------|
| `get_niagara_scratch_pad_scripts` | List scratch pad scripts with HLSL code |
| `create_niagara_scratch_pad_script` | Create new scratch pad (Module/DynamicInput/Function) |
| `delete_niagara_scratch_pad_script` | Delete scratch pad script |
| `set_niagara_scratch_pad_hlsl` | Write HLSL to existing scratch pad |
| `create_niagara_hlsl_module` | Create standalone module asset with custom HLSL, auto-attribute detection, word-boundary prefix replacement |
| `inspect_niagara_module` | Dump full module graph: nodes, pins, connections, HLSL code, metadata |

## Parameter Collections

| Tool | Description |
|------|-------------|
| `get_niagara_parameter_collection` | Read NPC parameters with values |
| `set_niagara_parameter_collection_value` | Set parameter value (float/int/bool) |
| `create_niagara_parameter_collection` | Create new NPC asset |
| `add_niagara_parameter_collection_param` | Add parameter (float/int/bool/vector/color) |
| `remove_niagara_parameter_collection_param` | Remove parameter |

## Scalability

| Tool | Description |
|------|-------------|
| `get_niagara_scalability` | Read emitter scalability settings and overrides |
| `set_niagara_scalability_override` | Set spawn count scale |

## Batch Operations

| Tool | Description |
|------|-------------|
| `batch_niagara_commands` | Execute multiple commands with single compile/save at end |

## Custom Vertices

| Tool | Description |
|------|-------------|
| `set_niagara_custom_vertices` | Set ribbon cross-section vertices |
| `get_niagara_custom_vertices` | Read ribbon cross-section vertices |
