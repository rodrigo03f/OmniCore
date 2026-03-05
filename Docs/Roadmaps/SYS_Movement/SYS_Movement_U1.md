# SYS_Movement_U1 — Execução e Evidências (Retroativo)

Status: CONCLUÍDO  
Data de fechamento: 2026-03-02  
System: SYS_Movement  
Update: U1 (Walk/Sprint + contratos canônicos)

---

## 1) Objetivo

Executar o núcleo mínimo de movimentação conforme o Design Spec:

- alternância determinística Walk/Sprint
- integração com ActionGate (permissão de sprint)
- integração com Status/Attributes (stamina/exhausted)
- contratos canônicos por tags (`Game.Movement.Mode.*`, `Game.State.Sprinting`, `Game.Movement.Intent.SprintRequested`)

Referência: `SYS_Movement_U1_Design_Spec.md`.

---

## 2) Escopo U1

✅ Inclui:
- Modo interno `Walk` e `Sprint`
- Publicação de tags de modo e estado
- Pipeline de velocidade efetiva (`BaseSpeed x SprintMultiplier`)
- Interrupção de sprint quando negado por gate ou quando exhausted
- Telemetria/logs Omni para inspeção em runtime

⛔ Não inclui:
- Crouch/jump/airborne avançado
- Root motion
- integrações de câmera/animação fora dos bridges canônicos

---

## 3) Evidências

- Build: `omni_build_dev.ps1` OK
- Conformance: `omni_conformance_gate.ps1` PASS
- Smoke headless: OK

Evidências de log (smoke 2026-03-05):
- `Saved/Logs/OmniSandbox.log:1035`
  - `[Omni][Runtime][Console] Sprint solicitada | Systems afetados=1`
- `Saved/Logs/OmniSandbox.log:1037`
  - `[Omni][Runtime][Console] Sprint cancelada | Systems afetados=1`

Evidências históricas em roadmap consolidado:
- `RM_InteligenciaSistemica.md` registra fechamento U1 com `build + gate + smoke` em 2026-03-02.

---

## 4) Commits / Histórico

- Validação registrada no fechamento da trilha U1 em `RM_InteligenciaSistemica`.
- Carimbo de referência citado no RM: `3b6d84e` (pós-fix com build/gate/smoke OK).
