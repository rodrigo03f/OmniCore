# OMNI B2.P0 Plan - CI Forge Gate

## Objective
Make OmniForge headless execution and determinism validation mandatory in CI before any B2 runtime-authority work.

Scope of this step:
- CI/workflow/scripts/config only
- No runtime code changes
- No Forge pipeline logic changes

## CI Gate Contract

Workflow: `.github/workflows/omni-conformance.yml`

Required jobs:
1. `omni-conformance-gate`
2. `omni-forge-gate`

`omni-forge-gate` must pass only if:
- Headless Forge run reports `Forge PASS` in log
- Expected artifact is generated
- Determinism compare script succeeds (two runs, same SHA256)

Failure conditions:
- `Forge FAIL` in log
- Missing artifact/report
- Timeout
- Compare hash mismatch
- Non-zero script exit code

## Runner/Environment Requirements

The Forge gate is configured for a Windows self-hosted runner with Unreal installed.

Configure one of:
- Repository variable `OMNI_UNREAL_EDITOR_CMD` with full `UnrealEditor-Cmd.exe` path, or
- Repository variable `OMNI_ENGINE_ROOT` pointing to the Unreal root folder

If neither is configured, the Forge gate fails fast with an explicit summary message.

## Local Validation

Single headless run:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\compare_omni_forge_headless.ps1 -SingleRunOnly
```

Determinism compare (two runs + SHA256 compare):

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\compare_omni_forge_headless.ps1
```

Useful optional args:
- `-EditorCmdPath "X:\...\UnrealEditor-Cmd.exe"`
- `-EngineRoot "D:\Epic Games\UE_5.7"`
- `-DiagnosticsDir "Saved/Logs/Automation/forge_headless"`
- `-RunTimeoutSeconds 900`

## CI Diagnostics and Artifacts

On failure, CI writes a step summary with:
- Missing files check
- Latest Forge diagnostic log path
- Tail (300 lines) of latest diagnostic log

CI uploads artifacts:
- `Saved/Logs/Automation/forge_headless/**`
- `Saved/Logs/*.log`
- `Saved/Logs/Automation/**`
- `Saved/Omni/ForgeReport.md`
- `Saved/Omni/ResolvedManifest.json` (if generated)
