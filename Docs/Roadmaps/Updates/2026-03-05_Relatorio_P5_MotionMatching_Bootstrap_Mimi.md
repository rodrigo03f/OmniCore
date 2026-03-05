# Relatório — P5 MotionMatching Bootstrap (Mimi)

Data: 2026-03-05  
RM: `RM_MotionMatching_RnD_P5`  
Status do recorte: **CONCLUÍDO (bootstrap técnico inicial)**

## Resumo executivo

Foi criado o módulo experimental `OmniExperimental` para R&D de Motion Matching, isolado do runtime canônico.  
O módulo compila no plugin Omni, expõe comandos de console `omni.mm` e registra estado/decisão placeholder sem alterar `OmniRuntime`, `SYS_AnimBridge` ou `SYS_AvatarBridge`.

## Entregas concluídas no recorte

- [PASS] Novo módulo Unreal: `Plugins/Omni/Source/OmniExperimental/`
- [PASS] Registro do módulo em `Plugins/Omni/Omni.uplugin`
- [PASS] Estrutura MotionMatching criada:
  - `Private/MotionMatching/OmniMotionMatchingSubsystem.h`
  - `Private/MotionMatching/OmniMotionMatchingSubsystem.cpp`
- [PASS] Estado mínimo implementado:
  - `FOmniMMState { bEnabled, ActiveAnimSetTag }`
- [PASS] Console commands:
  - `omni.mm enable 0|1`
  - `omni.mm status`
- [PASS] Placeholder de decisão implementado:
  - `Idle`, `Locomotion`, `Sprint`, `Exhausted` (por `Speed`, `bIsSprinting`, `bIsExhausted`)

## Evidências

- Build:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\omni_build_dev.ps1 -KillUnrealEditor`
  - Resultado: `Result: Succeeded`
- Smoke headless dos comandos MM:
  - ExecCmds: `omni.mm status, omni.mm enable 1, omni.mm status, quit`
  - Log: `Saved/Logs/Automation/MotionMatchingP5/bootstrap.log:989`
    - `[Omni][MM] enabled=false tag=Game.Anim.Set.MM.Debug ...`
  - Log: `Saved/Logs/Automation/MotionMatchingP5/bootstrap.log:990`
    - `[Omni][MM] enabled=true tag=Game.Anim.Set.MM.Debug ...`
  - Log: `Saved/Logs/Automation/MotionMatchingP5/bootstrap.log:991`
    - `[Omni][MM] enabled=true tag=Game.Anim.Set.MM.Debug ...`

## Restrições respeitadas

- [PASS] Nenhuma modificação em `OmniRuntime`
- [PASS] Nenhuma modificação em `SYS_AnimBridge`
- [PASS] Nenhuma modificação em `SYS_AvatarBridge`
- [PASS] Sem dependência circular nova

## Commit do recorte

- `b95c8b6`
- `feat(P5): bootstrap OmniExperimental module for Motion Matching R&D`

## Observação de contexto (alinhamento com Mimi)

O arquivo `Docs/Roadmaps/RM_MotionMatching_RnD_P5/RM_MotionMatching_RnD_P5.md` já estava com alterações locais no workspace e foi tratado como atualização externa válida (Mimi), não incluída neste commit de bootstrap técnico.

## Próximos passos sugeridos (P5 sequência)

1. Integrar saída canônica do decider (`ActiveAnimSetTag`) no contrato de animação sem acoplamento.
2. Adicionar smoke com `omni.sprint start/stop` para forçar transição `Idle -> Sprint -> Idle`.
3. Criar `MotionMatching_RnD_Report_P5.md` (arquitetura, riscos, custo/perf, plano de migração U2).
