"""Simulate Claude Code: launch an MCP server over stdio and send parallel tool calls.

Tests whether the MCP SDK + stdio transport on Windows can handle 3 concurrent
CallToolRequests, with no Unreal dependency.
"""
import subprocess
import sys
import json
import threading
import time
import os

MINIMAL = os.path.join(os.path.dirname(__file__), "..", "Python", "test_minimal_mcp.py")
UNREAL = os.path.join(os.path.dirname(__file__), "..", "Python", "unreal_mcp_server.py")
SCRIPT = UNREAL if "--unreal" in sys.argv else MINIMAL

def jsonrpc(id, method, params=None):
    obj = {"jsonrpc": "2.0", "id": id, "method": method}
    if params:
        obj["params"] = params
    return json.dumps(obj, separators=(",", ":")) + "\n"

def main():
    print(f"Launching minimal MCP server: {SCRIPT}")
    proc = subprocess.Popen(
        ["uv", "run", SCRIPT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        bufsize=0,
        cwd=os.path.dirname(SCRIPT),
    )

    responses = {}
    lock = threading.Lock()

    def read_stdout():
        """Read JSON-RPC responses from stdout."""
        while True:
            line = proc.stdout.readline()
            if not line:
                break
            try:
                msg = json.loads(line)
                rid = msg.get("id")
                with lock:
                    responses[rid] = msg
                    print(f"  [RECV id={rid}] {line.decode().strip()[:150]}")
            except:
                print(f"  [RECV raw] {line.decode().strip()[:150]}")

    reader = threading.Thread(target=read_stdout, daemon=True)
    reader.start()

    def send(msg):
        data = msg.encode("utf-8")
        proc.stdin.write(data)
        proc.stdin.flush()

    # Step 1: Initialize
    print("\n--- Initialize ---")
    send(jsonrpc("init-1", "initialize", {
        "protocolVersion": "2024-11-05",
        "capabilities": {},
        "clientInfo": {"name": "test", "version": "1.0"}
    }))
    time.sleep(1.0)
    send('{"jsonrpc":"2.0","method":"notifications/initialized"}\n')
    time.sleep(0.5)

    # Step 2: List tools
    print("\n--- List Tools ---")
    send(jsonrpc("list-1", "tools/list", {}))
    time.sleep(0.5)

    with lock:
        if "list-1" in responses:
            tools = responses["list-1"].get("result", {}).get("tools", [])
            print(f"  Tools found: {[t['name'] for t in tools]}")
        else:
            print("  WARNING: No tool list response!")

    # Step 3: Send 3 parallel tool calls AT ONCE
    print("\n--- Sending 3 parallel CallToolRequests ---")
    t0 = time.perf_counter()

    if "--unreal" in sys.argv:
        call1 = jsonrpc("call-1", "tools/call", {"name": "run_console_command", "arguments": {"command": "stat unit"}})
        call2 = jsonrpc("call-2", "tools/call", {"name": "find_actors_by_name", "arguments": {"pattern": "PlayerStart"}})
        call3 = jsonrpc("call-3", "tools/call", {"name": "get_material_info", "arguments": {"asset_path": "/Engine/BasicShapes/BasicShapeMaterial"}})
    else:
        call1 = jsonrpc("call-1", "tools/call", {"name": "tool_alpha", "arguments": {"msg": "one"}})
        call2 = jsonrpc("call-2", "tools/call", {"name": "tool_beta", "arguments": {"msg": "two"}})
        call3 = jsonrpc("call-3", "tools/call", {"name": "tool_gamma", "arguments": {"msg": "three"}})

    # Send all 3 in a single burst (no waiting between)
    burst = call1 + call2 + call3
    print(f"  Sending {len(burst)} bytes as single burst...")
    proc.stdin.write(burst.encode("utf-8"))
    proc.stdin.flush()

    # Wait for all 3 responses
    deadline = time.perf_counter() + 10.0
    while time.perf_counter() < deadline:
        with lock:
            got = sum(1 for k in responses if str(k).startswith("call-"))
        if got >= 3:
            break
        time.sleep(0.1)

    elapsed = time.perf_counter() - t0

    print(f"\n--- Results ({elapsed:.3f}s) ---")
    for rid in ["call-1", "call-2", "call-3"]:
        with lock:
            if rid in responses:
                r = responses[rid]
                content = r.get("result", {}).get("content", [{}])
                text = content[0].get("text", "") if content else ""
                print(f"  {rid}: OK — {text[:100]}")
            else:
                print(f"  {rid}: MISSING (not received within 10s)")

    with lock:
        ok = all(f"call-{i}" in responses for i in range(1, 4))

    print(f"\n{'PASS' if ok else 'FAIL'}: {'All 3' if ok else 'Not all'} responses received in {elapsed:.3f}s")

    # Print stderr for debugging
    proc.stdin.close()
    try:
        stderr_data = proc.stderr.read()
        if stderr_data:
            print(f"\n--- STDERR ---\n{stderr_data.decode('utf-8', errors='replace')[:2000]}")
    except:
        pass
    proc.wait(timeout=5)
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(main())
