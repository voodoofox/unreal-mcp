# Contributing

Thanks for hacking on Unreal MCP. The project has a few layers that have to stay in
sync, so the one thing worth internalizing is the **"adding a tool" checklist** below —
nothing enforces sync automatically, so it's easy to add a C++ command and forget the
wrapper, docs, or counts.

## Architecture in 30 seconds

- **C++ plugin** — `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/`. Each command is a
  `CommandType == TEXT("some_command")` branch in a `Private/Commands/UnrealMCP<Category>Commands.cpp`,
  declared in the matching `Public/Commands/*.h`, and routed from `Private/UnrealMCPBridge.cpp`.
- **Bridge** — the C++ side listens on TCP `127.0.0.1:55557`, **newline-delimited** JSON
  (`{"type": "command_name", "params": { ... }}\n`). Any command can be called directly over
  this socket.
- **Python MCP server** — `Python/`. Exposes a subset of commands as MCP tools. Each tool is an
  `@async_tool` wrapper in `Python/tools/<category>_tools.py` that calls
  `unreal.send_command("command_name", params)`. Modules are registered in `unreal_mcp_server.py`
  via `register_<category>_tools(mcp)`.
- **Docs** — `Docs/Tools/*.md` (per-category reference) + `README.md` (counts + highlights).

## Adding or changing a tool — the checklist

One command touches up to five places. Do all that apply, in the same change:

1. **C++ handler** — implement in the right `Private/Commands/UnrealMCP<Category>Commands.cpp`;
   declare it in `Public/Commands/UnrealMCP<Category>Commands.h`.
2. **Route it** — add the dispatch entry in `Private/UnrealMCPBridge.cpp`.
3. **Python wrapper** — add an `@async_tool` wrapper in `Python/tools/<category>_tools.py` so MCP
   clients see the tool. **Write a real docstring** — it is what AI agents read to decide how to
   call the tool; spell out non-obvious behavior, the build flow, and gotchas.
   - *Exception:* the granular Niagara commands deliberately have **no** 1:1 wrappers — they are
     driven through `import_niagara_system_spec` (spec → batch), `get_niagara_deep_inspect`, or
     direct TCP. Don't add one-off Niagara wrappers; keep them consistent.
4. **Docs** — add/update the row in `Docs/Tools/<category>_tools.md`.
5. **Counts** — update the tool counts in `README.md` (the badges **and** the "What Can It Do?"
   table) and in `Docs/Tools/README.md`.

Then **rebuild the plugin** and **restart the MCP server** (re-enable the connection) so both the
new C++ command and the new Python wrapper load, and smoke-test the call.

## Conventions

- **Names match verbatim** across all layers: the C++ string key, the Python wrapper function,
  and the docs all use the same `snake_case` command name. Parameter names match too.
- Floats are **doubles** in UE 5.7 Blueprints (`Multiply_DoubleDouble`, not `_FloatFloat`).
- Don't break the TCP framing: outgoing commands must end with `\n`, and never inject extra bytes
  (e.g. a ping byte) into the stream.
- After a C++ rebuild, restart the MCP connection (`/mcp disable unreal` then `/mcp enable unreal`,
  or your client's equivalent) so the server respawns with any new wrappers.

## See also

- [`Docs/Tools/README.md`](Docs/Tools/README.md) — tool index + the AI-agent invocation model.
- [`Python/README.md`](Python/README.md) — running the server.
