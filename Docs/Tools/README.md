# Unreal MCP Tools

Complete index of all 120+ tools available in the Unreal MCP plugin.

## Tool Categories

### Core (C++ command handlers)
- [Actor Tools](actor_tools.md) - spawn, delete, transform, inspect actors
- [Editor Tools](editor_tools.md) - console commands, asset management, Python execution
- [Blueprint Tools](blueprint_tools.md) - create blueprints, add components, set properties
- [Blueprint Node Tools](node_tools.md) - graph editing, variables, events, connections
- [Material & Inspection Tools](inspection_tools.md) - asset introspection, materials, meshes, Nanite, skeleton bones, BT inspection, MetaSound graphs
- [UMG Widget Tools](umg_tools.md) - widget blueprints, buttons, text blocks, bindings
- [Project Tools](project_tools.md) - input mappings, project settings
- [Niagara VFX Tools](niagara_tools.md) - 73 commands for full Niagara control including Custom HLSL modules

### Extended (Python bridge via `run_python_in_unreal`)
- [Animation Tools](animation_tools.md) - animation sequences, montages, skeletons, blend spaces
- [Audio Tools](audio_tools.md) - sound waves, cues, attenuation, audio components
- [DataTable Tools](datatable_tools.md) - list, inspect, and export DataTable rows
- [Level & World Tools](level_tools.md) - level info, world settings, streaming, actor summaries
- [AI Tools](ai_tools.md) - behavior trees, blackboards, MetaSounds, data channels

## Invocation model (for AI agents)

There are two ways the commands reach the editor:

1. **Registered MCP tools** — every tool in the categories above appears directly in your MCP tool list (via the Python `@async_tool` wrappers). Call them as normal MCP tools.
2. **Granular Niagara commands** — the ~73 individual Niagara commands are **not** 1:1 MCP tools. From an MCP client: read system state with `get_niagara_deep_inspect`, and build/modify a system with `import_niagara_system_spec` (a full spec applied as one batch). For one-off granular calls, send the command name + params as JSON over TCP to `127.0.0.1:55557` (newline-delimited) — the same bridge every tool uses. `run_python_in_unreal` is the escape hatch for anything no command covers.

Key workflow notes worth knowing up front:
- **UMG**: `create_umg_widget_blueprint` auto-creates a Canvas Panel named `RootCanvas` as the root — add children onto it. Use `umg_set_root_widget` only to swap in a *different* root panel.
- **Custom HLSL material nodes**: add inputs with `add_custom_node_input` (preserves existing inputs + connections); editing `Custom.Inputs` via ImportText/`set_material_expression_property` rebuilds the array and drops connections.
- **Niagara DI / curve edits** (`set_niagara_di_property`, `set_niagara_curve_data`) target the *source* data interface, so they survive a recompile and show in-editor.
- After rebuilding the plugin C++, restart the MCP server (re-enable the connection) so newly added tools load.

## Architecture

Tools are implemented in two layers:

1. **C++ command handlers** (`Plugins/UnrealMCP/Source/`) - native handlers for performance-critical operations like blueprint graph editing, material wiring, Niagara VFX, and actor management.

2. **Python bridge tools** (`Python/tools/`) - use the `run_python_in_unreal` escape hatch to execute Unreal Python API calls. Ideal for inspection and query tools. No C++ rebuild required.

All tools are registered as MCP tool functions and accessible from any MCP client (Claude Code, custom clients, etc.).
