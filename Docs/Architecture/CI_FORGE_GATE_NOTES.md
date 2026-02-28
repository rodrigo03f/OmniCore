# CI Forge Gate Notes

## Scope
- CI/workflow/scripts only
- No runtime changes
- No Forge runtime pipeline behavior changes

## Gate jobs
- `omni-conformance-gate`
- `omni-forge-gate`

`omni-forge-gate` performs:
1. Single headless Forge run (`-SingleRunOnly`)
2. Determinism compare (two runs + SHA256 compare of generated artifact)

## Runner requirements
- Windows self-hosted runner with Unreal Engine installed
- `UnrealEditor-Cmd.exe` must be resolvable by one of:
  - Repository variable `OMNI_UNREAL_EDITOR_CMD` (full path), or
  - Repository variable `OMNI_ENGINE_ROOT` (engine root)

If not resolvable, gate fails fast with explicit summary message.

## Local run

Single run:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\compare_omni_forge_headless.ps1 -SingleRunOnly
```

Determinism compare:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\compare_omni_forge_headless.ps1
```

## Common symptoms

1. `UnrealEditor-Cmd.exe not resolved/not found`
- Cause: missing `OMNI_UNREAL_EDITOR_CMD` and `OMNI_ENGINE_ROOT`, or invalid path.

2. Timeout
- Cause: headless run did not finish before configured timeout.

3. Missing artifact (`Saved/Omni/ResolvedManifest.json`)
- Cause: Forge run failed before artifact generation.

4. Hash mismatch
- Cause: compare run generated non-identical artifact content.

## Diagnostics location
- CI step summary includes:
  - explicit failure reason
  - missing file checks
  - tail of latest diagnostic log
- Uploaded artifacts include:
  - `Saved/Logs/Automation/forge_headless/**`
  - `Saved/Logs/*.log`
  - `Saved/Logs/Automation/**`
  - `Saved/Omni/ForgeReport.md`
  - `Saved/Omni/ResolvedManifest.json` (if generated)
