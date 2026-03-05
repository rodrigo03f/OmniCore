# RM_DocsHygiene_P3

## Status

CONCLUÍDO (retroativo)

Este RM documenta o hygiene pass realizado após conclusão de:
- RM_InteligenciaSistemica U1
- RM_PrototypePlayable P1
- RM_StatusAndModifiers P2 (Dia 1)

## Objetivo

Alinhar documentação do projeto com o estado real do código após
conclusão dos RMs anteriores e criação retroativa de documentação
faltante. Este RM **não altera código**, apenas documentação.

## Escopo

### 1. Atualização de status de RMs

Marcar como **CONCLUÍDO**: - RM_InteligenciaSistemica_U1 -
RM_PrototypePlayable_P1 - RM_StatusAndModifiers_P2 (Dia 1)

### 2. Correção no RM_StatusAndModifiers

Garantir que o pipeline descrito corresponda à implementação real.

SYS_Status U1 - duração determinística - stacks - tick determinístico

SYS_Modifiers U1 - pipeline Add / Mul - integração mínima Status →
Modifier

### 3. Remover duplicação em SYS_AvatarBridge_U1.md

-   manter apenas uma versão canônica
-   remover redundâncias

### 4. Criar documentação retroativa

Criar: SYS_Movement_U1.md

Conteúdo obrigatório: - Objetivo - Escopo - Contratos - Invariantes -
Execução - Integrações - Evidências - Critérios de aceite - Dívidas
conhecidas

### 5. Dívida conhecida

Pasta antiga: SYS_AnimBrigde

Status: resolvido no `RM_CharacterAnimPolish_P4` (renomeado para `SYS_AnimBridge`).

## Critérios de aceite

-   Nenhum .cpp ou .h alterado
-   RMs atualizados
-   SYS_Movement_U1.md criado
-   duplicação removida

## Commit esperado

docs: hygiene pass (RMs completed + movement retro doc)
