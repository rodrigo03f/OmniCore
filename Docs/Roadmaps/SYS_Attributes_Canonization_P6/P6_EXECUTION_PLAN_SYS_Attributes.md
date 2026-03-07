# P6 --- Canonização de SYS_Attributes separado de SYS_Status

Data: 2026-03-07

Decisão oficial:

-   SYS_Attributes = sistema dedicado de atributos numéricos
-   SYS_Status = sistema de efeitos temporários

Objetivo: alinhar a implementação do runtime com a arquitetura planejada
do Omni.

## Estrutura final desejada

Attributes → valores base (HP, Stamina, etc) Status → efeitos
temporários (Poison, Slow, etc) Modifiers → composição Systems
consumidores → Movement, ActionGate, Camera, UI

## Ordem de execução

PR A --- Introduzir SYS_Attributes (compat mode)

PR B --- Migrar contratos para Attributes

PR C --- Purificar SYS_Status

PR D --- Ajustar agregação de contexto

PR E --- Limpeza final e governança

Cada etapa deve:

-   compilar
-   manter runtime funcional
-   passar conformance gate
