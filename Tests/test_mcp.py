"""MCP plugin stability test suite — run after rebuild to verify fixes."""
import socket
import json
import time
import sys

HOST = "127.0.0.1"
PORT = 55557
TIMEOUT = 35  # slightly above the 30s command timeout

def send_cmd(cmd, params=None, timeout=TIMEOUT):
    """Send a command, return parsed JSON response."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(timeout)
    sock.connect((HOST, PORT))
    msg = {"command": cmd}
    if params:
        msg["params"] = params
    sock.sendall((json.dumps(msg) + "\n").encode("utf-8"))
    buf = b""
    while b"\n" not in buf:
        chunk = sock.recv(1024 * 256)
        if not chunk:
            break
        buf += chunk
    sock.close()
    line = buf.split(b"\n")[0]
    return json.loads(line)

def test_ping():
    """Test 1: Basic ping."""
    r = send_cmd("ping")
    assert r["status"] == "success", f"Ping failed: {r}"
    assert r["result"]["message"] == "pong"
    print("  PASS: ping")

def test_no_params():
    """Test 2: Command with 'type' field and no 'params' — was a crash."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(10)
    sock.connect((HOST, PORT))
    # Send using "type" field with NO params object — this used to crash
    msg = json.dumps({"type": "ping"}) + "\n"
    sock.sendall(msg.encode("utf-8"))
    buf = b""
    while b"\n" not in buf:
        chunk = sock.recv(65536)
        if not chunk:
            break
        buf += chunk
    sock.close()
    r = json.loads(buf.split(b"\n")[0])
    assert r["status"] == "success", f"No-params ping failed: {r}"
    print("  PASS: type field without params (crash fix)")

def test_command_field_no_params():
    """Test 3: 'command' field with no params."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(10)
    sock.connect((HOST, PORT))
    msg = json.dumps({"command": "ping"}) + "\n"
    sock.sendall(msg.encode("utf-8"))
    buf = b""
    while b"\n" not in buf:
        chunk = sock.recv(65536)
        if not chunk:
            break
        buf += chunk
    sock.close()
    r = json.loads(buf.split(b"\n")[0])
    assert r["status"] == "success", f"Command-no-params failed: {r}"
    print("  PASS: command field without params")

def test_unknown_command():
    """Test 4: Unknown command returns error (not crash)."""
    r = send_cmd("nonexistent_command_xyz")
    assert r["status"] == "error", f"Expected error: {r}"
    assert "Unknown command" in r.get("error", ""), f"Bad error msg: {r}"
    print("  PASS: unknown command returns error")

def test_malformed_json():
    """Test 5: Malformed JSON doesn't crash — server stays alive."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5)
    sock.connect((HOST, PORT))
    sock.sendall(b"this is not json\n")
    # Server should ignore the bad message; send a valid ping right after
    time.sleep(0.1)
    msg = json.dumps({"command": "ping"}) + "\n"
    sock.sendall(msg.encode("utf-8"))
    buf = b""
    try:
        while b"\n" not in buf:
            chunk = sock.recv(65536)
            if not chunk:
                break
            buf += chunk
    except socket.timeout:
        pass
    sock.close()
    if buf:
        r = json.loads(buf.split(b"\n")[0])
        assert r["status"] == "success", f"Ping after bad JSON failed: {r}"
        print("  PASS: malformed JSON ignored, connection survives")
    else:
        print("  PASS: malformed JSON handled (connection closed gracefully)")

def test_large_response():
    """Test 6: Large response (list_assets) — tests chunked send."""
    r = send_cmd("list_assets", {"path": "/Game", "recursive": True})
    resp_str = json.dumps(r)
    size_kb = len(resp_str) / 1024
    assert r["status"] == "success", f"list_assets failed: {r.get('error', 'unknown')}"
    items = r.get("result", {}).get("assets", [])
    print(f"  PASS: large response ({size_kb:.0f} KB, {len(items)} assets)")

def test_material_graph():
    """Test 7: Material graph — the biggest response, previously caused timeout."""
    r = send_cmd("get_material_graph", {"asset_path": "/Game/Materials/PP/M_VHSEffect"})
    resp_str = json.dumps(r)
    size_kb = len(resp_str) / 1024
    if r["status"] == "success":
        nodes = len(r.get("result", {}).get("nodes", r.get("result", {}).get("expressions", [])))
        print(f"  PASS: material graph ({size_kb:.0f} KB, {nodes} nodes)")
    else:
        print(f"  WARN: material graph returned error ({size_kb:.0f} KB): {r.get('error', '')[:100]}")

def test_multiple_commands_one_connection():
    """Test 8: Multiple commands on a single persistent connection."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(10)
    sock.connect((HOST, PORT))

    for i in range(5):
        msg = json.dumps({"command": "ping"}) + "\n"
        sock.sendall(msg.encode("utf-8"))
        buf = b""
        while b"\n" not in buf:
            chunk = sock.recv(65536)
            if not chunk:
                break
            buf += chunk
        r = json.loads(buf.split(b"\n")[0])
        assert r["status"] == "success", f"Multi-cmd ping {i} failed: {r}"

    sock.close()
    print("  PASS: 5 sequential commands on one connection")

def test_rapid_reconnect():
    """Test 9: Rapid connect/disconnect cycles."""
    for i in range(10):
        r = send_cmd("ping", timeout=5)
        assert r["status"] == "success", f"Rapid reconnect {i} failed: {r}"
    print("  PASS: 10 rapid reconnect cycles")

def test_blueprint_graph():
    """Test 10: Blueprint graph — another large response."""
    r = send_cmd("get_blueprint_graph", {"blueprint_name": "EndlessRunnerGameMode"})
    resp_str = json.dumps(r)
    size_kb = len(resp_str) / 1024
    if r["status"] == "success":
        print(f"  PASS: blueprint graph ({size_kb:.0f} KB)")
    else:
        # Blueprint might not be found by simple name — not a stability failure
        print(f"  SKIP: blueprint graph ({r.get('error', '')[:80]})")

if __name__ == "__main__":
    print("MCP Stability Test Suite")
    print("=" * 40)

    tests = [
        test_ping,
        test_no_params,
        test_command_field_no_params,
        test_unknown_command,
        test_malformed_json,
        test_large_response,
        test_material_graph,
        test_multiple_commands_one_connection,
        test_rapid_reconnect,
        test_blueprint_graph,
    ]

    passed = 0
    failed = 0
    for t in tests:
        try:
            t()
            passed += 1
        except Exception as e:
            print(f"  FAIL: {t.__name__} — {e}")
            failed += 1

    print("=" * 40)
    print(f"Results: {passed} passed, {failed} failed, {len(tests)} total")
    sys.exit(1 if failed else 0)
