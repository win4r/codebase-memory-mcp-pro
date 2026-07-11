#!/bin/sh
# Verify the installed executable against a sealed personal-plugin contract.
# This intentionally fails before calling --manifest when --version is not a
# traceable semantic build, so a legacy `dev` executable cannot hang as MCP.

set -eu

binary=${1:-"$HOME/.local/bin/codebase-memory-mcp"}
plugin_dir=${2:-"$HOME/plugins/codebase-memory-mcp-pro"}
contract="$plugin_dir/plugin-contract.json"

if [ ! -x "$binary" ]; then
    printf 'plugin smoke failed: executable is missing or not executable: %s\n' "$binary" >&2
    exit 1
fi
if [ ! -f "$contract" ]; then
    printf 'plugin smoke failed: contract is missing: %s\n' "$contract" >&2
    exit 1
fi

actual_sha=$(shasum -a 256 "$binary" | awk '{print $1}')
version_output=$("$binary" --version 2>&1)

python3 - "$binary" "$contract" "$actual_sha" "$version_output" <<'PY'
import json
import re
import subprocess
import sys

binary, contract_path, actual_sha, version_output = sys.argv[1:]
contract = json.load(open(contract_path, encoding="utf-8"))
binary_contract = contract["binary"]
capabilities = contract["capabilities"]

version_re = re.compile(
    r"^codebase-memory-mcp "
    r"(?P<version>\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?) "
    r"commit=(?P<commit>[0-9a-f]{7,40}) "
    r"dirty=(?P<dirty>clean|dirty) "
    r"build=(?P<build>[0-9A-Za-z._-]+)$"
)
match = version_re.fullmatch(version_output.strip())
if not match:
    raise SystemExit(
        "plugin smoke failed: --version must expose semantic version, commit, "
        "dirty state, and build id; got: " + version_output.strip()
    )

for key in ("version", "commit", "dirty", "build_id"):
    expected = binary_contract["expected_" + key]
    actual = match.group("build" if key == "build_id" else key)
    if expected != actual:
        raise SystemExit(
            f"plugin smoke failed: {key} mismatch (expected {expected}, got {actual})"
        )

expected_sha = binary_contract["sha256"]
if expected_sha == "PENDING_REBUILD":
    raise SystemExit(
        "plugin smoke failed: plugin contract is unsealed; rebuild from the recorded "
        "source state and write the installed SHA-256 before activation"
    )
if expected_sha != actual_sha:
    raise SystemExit(
        f"plugin smoke failed: SHA-256 mismatch (expected {expected_sha}, got {actual_sha})"
    )

manifest = subprocess.run(
    [binary, "--manifest"], text=True, capture_output=True, timeout=10, check=False
)
if manifest.returncode != 0:
    raise SystemExit("plugin smoke failed: --manifest failed: " + manifest.stderr.strip())
try:
    actual = json.loads(manifest.stdout)
except json.JSONDecodeError as exc:
    raise SystemExit("plugin smoke failed: --manifest did not return JSON: " + str(exc))

actual_capabilities = actual.get("capabilities", {})
if actual_capabilities.get("schema_version") != contract["schema_version"]:
    raise SystemExit("plugin smoke failed: capability schema version mismatch")
if actual_capabilities.get("tool_count") != capabilities["tool_count"]:
    raise SystemExit("plugin smoke failed: tool count mismatch")
if actual_capabilities.get("ui_assets") is not capabilities["ui_assets"]:
    raise SystemExit("plugin smoke failed: UI-assets capability mismatch")
if actual_capabilities.get("tools") != capabilities["tools"]:
    raise SystemExit("plugin smoke failed: tool list/schema drift")

print("plugin smoke passed: version, checksum, tools, and UI capability match")
PY
