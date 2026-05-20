# Animation Tools

Tools for inspecting animation assets, skeletal meshes, and animation state on actors.

## list_animation_assets

List animation assets in the content browser.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `path` | string | `/Game` | Content browser path to search |
| `asset_type` | string | `AnimSequence` | `AnimSequence`, `AnimMontage`, `AnimBlueprint`, `BlendSpace`, `BlendSpace1D` |
| `recursive` | bool | `true` | Search subdirectories |

## get_animation_info

Get detailed info about an AnimSequence or AnimMontage  - duration, frame count, rate scale, skeleton reference, montage sections.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Full asset path (e.g. `/Game/Animations/Walk`) |

## get_skeleton_info

Get bone hierarchy from a Skeleton or SkeletalMesh asset.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `asset_path` | string | | Path to Skeleton or SkeletalMesh |
| `max_bones` | int | `200` | Max bones to return |

Returns bone index, name, and parent name for each bone.

## get_blend_space_info

Get BlendSpace info  - axis settings, skeleton reference, duration.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Path to BlendSpace or BlendSpace1D |

## get_actor_animation_state

Get current animation state of a skeletal mesh actor in the level  - current mesh, animation mode, anim blueprint, playing animation.

| Parameter | Type | Description |
|-----------|------|-------------|
| `actor_name` | string | Name or label of the actor |

## set_actor_animation

Set an animation on a skeletal mesh actor for preview in the editor.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `actor_name` | string | | Name of the actor |
| `animation_path` | string | | Path to AnimSequence |
| `looping` | bool | `true` | Loop the animation |

## get_animation_notifies

Get animation notify events from an AnimSequence or AnimMontage.

| Parameter | Type | Description |
|-----------|------|-------------|
| `asset_path` | string | Path to animation asset |
