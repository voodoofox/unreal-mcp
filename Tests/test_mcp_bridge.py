"""Test the Python MCP bridge layer (unreal_mcp_server.py UnrealConnection)."""
import sys
sys.path.insert(0, r"C:\UnrealMCP\unreal-mcp-main\Python")

import json
from unreal_mcp_server import UnrealConnection

def test_bridge_ping():
    conn = UnrealConnection()
    r = conn.send_command("ping")
    assert r and r.get("status") == "success", f"Bridge ping failed: {r}"
    print("  PASS: bridge ping")

def test_bridge_no_params():
    conn = UnrealConnection()
    r = conn.send_command("ping", None)
    assert r and r.get("status") == "success", f"Bridge no-params failed: {r}"
    print("  PASS: bridge ping (no params)")

def test_bridge_unknown():
    conn = UnrealConnection()
    r = conn.send_command("nonexistent_xyz")
    assert r and r.get("status") == "error", f"Expected error: {r}"
    print("  PASS: bridge unknown command returns error")

def test_bridge_large():
    conn = UnrealConnection()
    r = conn.send_command("list_assets", {"path": "/Game", "recursive": True})
    assert r and r.get("status") == "success", f"Bridge large response failed: {r}"
    assets = r.get("result", {}).get("assets", [])
    print(f"  PASS: bridge large response ({len(assets)} assets)")

def test_bridge_material_graph():
    conn = UnrealConnection()
    r = conn.send_command("get_material_graph", {"asset_path": "/Game/Materials/PP/M_VHSEffect"})
    assert r and r.get("status") == "success", f"Bridge material graph failed: {r}"
    size_kb = len(json.dumps(r)) / 1024
    print(f"  PASS: bridge material graph ({size_kb:.0f} KB)")

def test_bridge_rapid():
    for i in range(10):
        conn = UnrealConnection()
        r = conn.send_command("ping")
        assert r and r.get("status") == "success", f"Rapid {i} failed: {r}"
    print("  PASS: bridge 10 rapid sequential commands")

if __name__ == "__main__":
    print("MCP Python Bridge Test Suite")
    print("=" * 40)

    tests = [
        test_bridge_ping,
        test_bridge_no_params,
        test_bridge_unknown,
        test_bridge_large,
        test_bridge_material_graph,
        test_bridge_rapid,
    ]

    passed = 0
    failed = 0
    for t in tests:
        try:
            t()
            passed += 1
        except Exception as e:
            print(f"  FAIL: {t.__name__} -- {e}")
            failed += 1

    print("=" * 40)
    print(f"Results: {passed} passed, {failed} failed, {len(tests)} total")
    sys.exit(1 if failed else 0)
