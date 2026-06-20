# UMG Widget Tools

Tools for creating and manipulating UMG Widget Blueprints.

## create_umg_widget_blueprint

Create a new UMG Widget Blueprint. Creates a real `UWidgetBlueprint` and auto-adds a Canvas Panel named `RootCanvas` as the root, so Python builders can find and add children to it. Returns `parent_class` and `root_widget` (`"RootCanvas"`).

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `widget_name` | string | | Name of the widget blueprint |
| `parent_class` | string | `UserWidget` | Parent class — bare name, `U`-prefixed, or full object path (must be a `UserWidget` subclass) |
| `path` | string | `/Game/UI` | Content browser path |

## add_text_block_to_widget

Add a Text Block to a widget.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `widget_name` | string | | Target Widget Blueprint |
| `text_block_name` | string | | Name for the Text Block |
| `text` | string | `""` | Initial text |
| `position` | float[2] | `[0, 0]` | Canvas position |
| `size` | float[2] | `[200, 50]` | Width, height |
| `font_size` | int | `12` | Font size in points |
| `color` | float[4] | `[1,1,1,1]` | RGBA color |

## add_button_to_widget

Add a Button to a widget.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `widget_name` | string | | Target Widget Blueprint |
| `button_name` | string | | Name for the Button |
| `text` | string | `""` | Button label |
| `position` | float[2] | `[0, 0]` | Canvas position |
| `size` | float[2] | `[200, 50]` | Width, height |
| `font_size` | int | `12` | Font size |
| `color` | float[4] | `[1,1,1,1]` | Text RGBA |
| `background_color` | float[4] | `[0.1,0.1,0.1,1]` | Background RGBA |

## bind_widget_event

Bind a widget component event (e.g. OnClicked) to a function.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `widget_name` | string | | Target Widget Blueprint |
| `widget_component_name` | string | | Component name (button, etc.) |
| `event_name` | string | | Event to bind (OnClicked, etc.) |
| `function_name` | string | auto | Function to create/bind to |

## add_widget_to_viewport

Add a Widget Blueprint instance to the viewport.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `widget_name` | string | | Widget Blueprint to add |
| `z_order` | int | `0` | Z-order (higher = on top) |

## set_text_block_binding

Set up a property binding for a Text Block.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `widget_name` | string | | Target Widget Blueprint |
| `text_block_name` | string | | Text Block to bind |
| `binding_property` | string | | Property to bind to |
| `binding_type` | string | `Text` | Binding type (Text, Visibility, etc.) |

## umg_set_root_widget

Promote an existing widget (already present in the WidgetTree) to be the tree's Root Widget, then compile. Needed because `RootWidget` is a protected property that can't be set from Python. Returns `root_widget`.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `blueprint_name` | string | | Widget Blueprint name (resolved under `/Game/Widgets/`) or full asset path |
| `widget_name` | string | | Name of the widget already in the tree to make root |
