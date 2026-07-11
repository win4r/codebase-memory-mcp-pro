# Local build provenance

## Scope

This file records the verified boundary between the clean local feature branch,
the preserved pre-cleanup snapshot, the installed executable, and the personal
Codex plugin. The original dirty state was committed to a local safety branch;
the clean branch was rebuilt from the upstream base with only modified and added
paths replayed.

## Verified 2026-07-11

| Surface | Identity | Verification |
| --- | --- | --- |
| Clean source branch | `codex/codebase-memory-p0-clean`, based on `3ff76d29a639c5be2ca27c943ce2f87de141b73c` | 45 modified/added paths replayed; zero tracked deletions |
| Pre-cleanup safety snapshot | `codex/dirty-snapshot-20260711` at `b8b1108087d5327f89a3310f88c3660b5860ca5d` | Preserves the complete original dirty state, including prior deletions |
| Installed executable | `/Users/wangyukai/.local/bin/codebase-memory-mcp` | SHA-256 `158534554ce54819b0222658a43f27c7c110b5f0afbaaafdef3073407efaad1d` |
| Installed build identity | `0.8.1-dev` | Build ID `p0.20260711.3ff76d29a639.dirty.r2` |
| Previous executable backup | `/Users/wangyukai/.local/bin/codebase-memory-mcp.pre-p0-20260711` | SHA-256 `663fefe9d30bfb6602819bb85f103e3ce10e591e4c54c7a2e51b38e97163f911` |
| Personal plugin source | `/Users/wangyukai/plugins/codebase-memory-mcp-pro` | Contract smoke passed |
| Installed plugin cache | `/Users/wangyukai/.codex/plugins/cache/personal/codebase-memory-mcp-pro/0.1.0+codex.20260628162445` | Contract smoke passed |

The installed executable's `--manifest` is the runtime capability source of
truth. It reports schema version 1, 17 tools, `ui_assets=false`, `explore`,
`cancel_index`, and `prune_index`; it does not advertise `repo_map`.

## P0 verification

- Production build passed `-Wall -Wextra -Werror`.
- The ASan/UBSan test runner completed with `5638 passed` and no failures.
- Background indexing returned observable job progress and explicit
  cancellation completed without terminating the MCP server.
- Freshness runtime checks demonstrated `ready`, `stale_source`, and
  `missing_root`; automated tests also cover `stale_extractor` and `partial`.
- `prune_index` is dry-run only. `dry_run=false` was rejected, and the test
  database SHA-256 remained unchanged before and after prune checks.
- Both plugin contracts matched the installed version, commit, dirty state,
  build ID, SHA-256, exact tool order, tool count, and UI capability.
- The clean branch was independently rebuilt in fresh `/tmp` directories: the
  ASan/UBSan runner completed with `5638 passed`, and the production `-Werror`
  build completed successfully with a 17-tool, schema-1 manifest.
- The local shell's inherited `TMPDIR` did not exist, so clean verification used
  `TMPDIR=/tmp`; the initial failure was environmental and occurred before tests.

Existing MCP processes were deliberately not killed during installation. They
continue running their previously loaded executable image until Codex starts a
fresh MCP process; on-disk verification applies to new processes.

## Contract rule

Every future install must retain all of the following before plugin activation:

1. semantic version, commit, dirty state, and build identifier;
2. installed executable SHA-256;
3. machine-readable `--manifest` from that executable;
4. a smoke test comparing both plugin contract copies to the executable.

Do not run `codebase-memory-mcp update` for this local fork build; it can replace
the verified local executable with an upstream release.
