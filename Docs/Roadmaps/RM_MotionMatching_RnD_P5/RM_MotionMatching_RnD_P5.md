# RM_MotionMatching_RnD_P5

Status: PLANEJADO\
Data: 2026-03-05\
Tipo: R&D (Research & Development) --- **isolado**

------------------------------------------------------------------------

## 0) Resumo (1 parágrafo)

Investigar viabilidade de **Motion Matching** (ou PoseSearch / seleção
de poses) dentro da arquitetura Omni **sem contaminar o runtime
canônico**.\
O resultado do P5 é um **módulo experimental** com protótipo mínimo +
relatório técnico + smoke headless, com integração via **contrato** com
o `SYS_AnimBridge` (bridge aplica; R&D decide).

------------------------------------------------------------------------

## 1) Princípios / Regras do P5

1.  **Não mexer no OmniRuntime canônico** (exceto, no máximo, adicionar
    um *hook* de leitura/saída canônica **se for inevitável** e for
    aprovado).
2.  O R&D vive em **módulo isolado**: `OmniExperimental`.
3.  Nenhuma dependência circular:
    -   `AnimBP` não consulta Systems.
    -   `SYS_AnimBridge` não vira "cérebro" de motion matching.
4.  Falhar é permitido: código pode ser descartado. O aprendizado vira
    doc.

------------------------------------------------------------------------

## 2) Local (onde vai viver no repo)

**Código (módulo):** - `Plugins/Omni/Source/OmniExperimental/` -
`Public/` - `Private/` - (subpasta sugerida) `Private/MotionMatching/`

**Docs (roadmap + relatório):** -
`Plugins/Omni/Docs/Roadmaps/RM_MotionMatching_RnD_P5.md` (este
arquivo) - `Plugins/Omni/Docs/RnD/MotionMatching_RnD_Report_P5.md`

------------------------------------------------------------------------

## 3) Interface mínima (contrato) --- versão P5

### Inputs mínimos (o que o R&D precisa ler)

-   `Speed` (float)
-   `bIsSprinting` (bool)
-   `bIsExhausted` (bool)
-   `bIsInAir` (bool) --- pode vir do fallback CMC já usado pelo
    AnimBridge
-   `MoveDirection` (opcional no P5; pode ser derivado depois)

Fonte sugerida: - `UOmniAnimBridgeComponent` / `UOmniAnimInstanceBase`
(estado já consolidado pelo bridge) - ou snapshot de Movement/Status (se
já existir um caminho canônico fácil)

### Output mínimo (o que o R&D produz)

-   `ActiveAnimSetTag` (GameplayTag)
    -   Ex.: `Game.Anim.Set.MM.Debug`

Motivo: `ActiveAnimSetTag` já é um canal canônico simples para validar
"decisão de animação" sem PoseSearch completo.

------------------------------------------------------------------------

## 4) Escopo do protótipo P5 (MVP R&D)

### 4.1 --- Criar módulo `OmniExperimental`

-   [ ] Criar `OmniExperimental.Build.cs`
-   [ ] Adicionar módulo no `.uplugin`
-   [ ] Build OK

### 4.2 --- Criar um "decider" simples (sem PoseSearch real ainda)

Implementar uma lógica provisória (placeholder) só para provar o fluxo:

-   se `bIsExhausted` → `Game.Anim.Set.MM.Exhausted`
-   senão se `bIsSprinting` → `Game.Anim.Set.MM.Sprint`
-   senão se `Speed > 0` → `Game.Anim.Set.MM.Locomotion`
-   senão → `Game.Anim.Set.MM.Idle`

### 4.3 --- Integração via contrato com o AnimBridge (sem acoplamento)

-   [ ] O decider escreve `ActiveAnimSetTag` em um local canônico:
    -   opção A (preferida no P5): escreve no `UOmniAnimInstanceBase`
        (via uma função/entrypoint do bridge)
    -   opção B: escreve em um "output struct" consumido pelo bridge
-   [ ] O `SYS_AnimBridge` continua apenas aplicando frame no
    AnimInstance (não decide).

### 4.4 --- Console commands (para operar headless)

Adicionar comandos no módulo experimental (ou reusar console infra já
existente):

-   [ ] `omni.mm enable 0|1`
-   [ ] `omni.mm status`
-   [ ] (opcional) `omni.mm setmode <Idle|Locomotion|Sprint|Exhausted>`
    para forçar modo e validar HUD/Anim

### 4.5 --- Smoke headless (evidência)

Criar 1 smoke headless mínimo:

-   ExecCmds sugeridos:
    -   `omni.mm enable 1`
    -   `omni.mm status`
    -   `omni.sprint start`
    -   `omni.mm status`
    -   `omni.sprint stop`
    -   `omni.mm status`
    -   `quit`

Logs esperados (exemplos): -
`[Omni][MM][Status] enabled=True mode=Locomotion tag=Game.Anim.Set.MM.Locomotion` -
`[Omni][MM][Status] enabled=True mode=Sprint tag=Game.Anim.Set.MM.Sprint`

------------------------------------------------------------------------

## 5) Fora de escopo (explicitamente)

-   PoseSearch / database real
-   Trajectory variables
-   IK avançado
-   Motion matching "real" com custo/perf
-   Integrar em systems canônicos (U2 only, após avaliação)

------------------------------------------------------------------------

## 6) Critérios de aceite do P5

-   [ ] `OmniExperimental` compila e não quebra o plugin
-   [ ] `omni.mm status` funciona
-   [ ] Smoke headless executa e gera logs coerentes
-   [ ] Documento `MotionMatching_RnD_Report_P5.md` criado com:
    -   arquitetura proposta
    -   riscos
    -   custo/perf (chute inicial + pontos a medir)
    -   plano de migração (se virar U2)

------------------------------------------------------------------------

## 7) Entregáveis

1.  Código: `OmniExperimental` + decider mínimo + console commands
2.  Docs: este RM
3.  Relatório: `MotionMatching_RnD_Report_P5.md`
4.  Evidências: logs do smoke (path padrão do projeto)

------------------------------------------------------------------------

## 8) Próximo passo após P5 (se der certo)

-   Propor `SYS_MotionMatching_U2` (ou equivalente) **ainda isolado**,
    com:
    -   contrato formal de inputs/outputs
    -   testes de performance
    -   integração gradual com anim pipeline
