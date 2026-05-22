# AI, Behavior Tree & MetaSound Tools

Tools for inspecting AI assets (Behavior Trees, Blackboards) and audio systems (MetaSounds, Data Channels).

## Behavior Trees

### list_behavior_trees

List all Behavior Tree assets under a content browser path.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | string | `/Game` | Content browser path |
| `recursive` | bool | `true` | Search subdirectories |

### get_behavior_tree_info

Get full Behavior Tree structure using native C++ traversal - composites, tasks, decorators per child, blackboard keys with types.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Full asset path to the BehaviorTree |

### list_blackboard_assets

List all Blackboard Data assets under a content browser path.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | string | `/Game` | Content browser path |
| `recursive` | bool | `true` | Search subdirectories |

## MetaSounds

### list_metasound_assets

List all MetaSound source assets.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | string | `/Game` | Content browser path |
| `recursive` | bool | `true` | Search subdirectories |

### get_metasound_info

Get basic MetaSound asset info via Python reflection.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Path to MetaSound asset |

### get_metasound_graph

Get full MetaSound graph via native C++ - all nodes with inputs/outputs, all edges/connections, root interface.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Path to MetaSoundSource asset |

## Data Channels

### list_niagara_data_channels

List Niagara Data Channel assets.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | string | `/Game` | Content browser path |
| `recursive` | bool | `true` | Search subdirectories |

### get_niagara_data_channel_info

Inspect a data channel asset - channel type, variables, editable properties.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Path to NiagaraDataChannelAsset |
