# SYS_ActionGate_U1

Objetivo:
- Introduzir FDecision.
- Implementar Can(ActionTag, ContextTags).
- Implementar Locks determinísticos por tag.

Fora de escopo:
- Rule composition completa.
- Cooldowns.
- Integração via Query.

Critérios de aceite:
- Nunca retornar bool cru.
- Snapshot ordenado.
- Build OK.
- Gate OK.
- Smoke OK.

## Entregas U1 (Canonico)

- `OmniGateTypes.h` criado em `Public` com:
  - `FOmniGateDecision { bAllowed, ReasonTag, ReasonText }`
  - `FOmniGateLock { LockTag, SourceId, Scope, ActionTag }`
  - `EOmniActionGateLockScope { Global, Action }`
- API canônica implementada em `UOmniActionGateSystem`:
  - `Can(ActionTag, ContextTags) -> FOmniGateDecision`
  - `AddLock/RemoveLock/GetActiveLockCount`
  - `GetActiveLockSnapshot` com ordenação estável
- Prioridade de lock aplicada: `Global > Action`
- Reason tags oficiais aplicadas em runtime:
  - `Omni.Gate.Allow.Ok`
  - `Omni.Gate.Deny.Locked`
- Query `CanStartAction` migrou para payload canônico:
  - `ReasonTag`
  - `ReasonText`

## Evidências (02-03-2026)

- Build incremental:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_build_dev.ps1`
  - Resultado: `Succeeded`
- Conformance gate:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_conformance_gate.ps1`
  - Resultado: `PASSED`
- Smoke headless mínimo:
  - `UnrealEditor-Cmd ... -ExecCmds="omni.sprint status;quit"`
  - Evidência em `Saved/Logs/OmniSandbox.log`: runtime inicializou `ActionGate` e executou `omni.sprint status`.
