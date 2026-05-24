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
- [Niagara VFX Tools](niagara_tools.md) - 71 commands for full Niagara control including Custom HLSL modules

### Extended (Python bridge via `run_python_in_unreal`)
- [Animation Tools](animation_tools.md) - animation sequences, montages, skeletons, blend spaces
- [Audio Tools](audio_tools.md) - sound waves, cues, attenuation, audio components
- [DataTable Tools](datatable_tools.md) - list, inspect, and export DataTable rows
- [Level & World Tools](level_tools.md) - level info, world settings, streaming, actor summaries
- [AI Tools](ai_tools.md) - behavior trees, blackboards, MetaSounds, data channels

## Architecture

Tools are implemented in two layers:

1. **C++ command handlers** (`Plugins/UnrealMCP/Source/`) - native handlers for performance-critical operations like blueprint graph editing, material wiring, Niagara VFX, and actor management.

2. **Python bridge tools** (`Python/tools/`) - use the `run_python_in_unreal` escape hatch to execute Unreal Python API calls. Ideal for inspection and query tools. No C++ rebuild required.

All tools are registered as MCP tool functions and accessible from any MCP client (Claude Code, custom clients, etc.).
