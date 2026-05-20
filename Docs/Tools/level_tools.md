# Level & World Tools

Tools for inspecting levels, world settings, streaming configuration, and actor summaries.

## get_current_level_info

Get info about the currently open level  - name, path, actor count, actor class breakdown, game mode.

No parameters.

## list_levels

List all Level (map) assets under a content browser path.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | string | `/Game` | Content browser path |
| `recursive` | bool | `true` | Search subdirectories |

## get_world_settings

Get the current level's WorldSettings  - default game mode, world bounds checks, kill Z.

No parameters.

## get_level_streaming_info

Get streaming level configuration  - sub-levels, their types, load/visibility status, transforms.

No parameters.

## open_level

Open a level in the editor.

| Parameter | Type | Description |
|-----------|------|-------------|
| `level_path` | string | Content path to the level (e.g. `/Game/Maps/MyLevel`) |

## get_level_actor_summary

Get a summary of actors in the current level with location data, optionally filtered by class.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `class_filter` | string | `""` | Only include actors of this class (e.g. `StaticMeshActor`, `PointLight`). Empty = all. |

Returns up to 500 actors with name, label, class, and location.
