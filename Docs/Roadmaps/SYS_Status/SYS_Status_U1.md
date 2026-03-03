# SYS_Status_U1 — Execução e Evidências

Status: CONCLUÍDO  
Data: 2026-03-03  
System: SYS_Status  
Update: U1 (buff/debuff determinístico)

---

## 1) Escopo U1

- Apply/Remove por `StatusTag + SourceId`
- Duração determinística via `OmniClock`
- Stacks determinísticos (`MaxStacks` + `RefreshPolicy`)
- Tick determinístico por intervalo estável
- Snapshot ordenado por `StatusTag` e `SourceId`
- Logs por evento (`Apply`, `Expire`, `StackChanged`) sem spam por tick

---

## 2) Checklist

- [x] Runtime: `ApplyStatus` / `RemoveStatus`
- [x] Duração + expiração por `OmniClock`
- [x] Stack policy determinística
- [x] Tick interval determinístico
- [x] Snapshot ordenado e queryável
- [x] Smoke: aplicar 1 status, expirar, aplicar stack

---

## 3) Evidências

- [x] Build OK (`omni_build_dev.ps1`)
  - Resultado: `Result: Succeeded` em 2026-03-03.
- [x] Gate PASS (`omni_conformance_gate.ps1`)
  - Resultado: `Conformance gate PASSED. No B1/B0 conformance violations found.` em 2026-03-03.
- [x] Smoke OK (headless)
  - Execução headless com janela de 18s para permitir expiração:
    - `-ExecCmds="omni.statusfx apply Game.Status.Burning 1.50 Console,omni.statusfx apply Game.Status.Burning 1.50 Console,omni.statusfx status"`
  - Evidência de logs:
    - `[Omni][Status][Event] Apply | status=Game.Status.Burning source=Console duration=1.50 stacks=1`
    - `[Omni][Status][Event] StackChanged | status=Game.Status.Burning source=Console stacks=2`
    - `[Omni][Status][Event] Expire | status=Game.Status.Burning source=Console reason=Duration`

---

## 4) Commit

`feat(SYS_Status_U1): deterministic status effects (duration/stack/tick)`
