# Plano Tecnico de Canonizacao - SYS_Attributes separado de SYS_Status

Data: 2026-03-07  
Decisao oficial: `SYS_Attributes` deve ser system separado; `SYS_Status` deve cuidar apenas de status temporarios (duracao, stacks, tick, tags/efeitos).

## 1) O que hoje esta em Status e deveria ir para Attributes

- Estado de atributos numericos:
  - `AttributesByTag`, `HP`, `Stamina`, `FOmniAttributeSnapshot`
  - Arquivos atuais: `OmniStatusSystem.h/.cpp`
- Loop de stamina e exhausted:
  - `TickStamina`, `SetExhausted`, thresholds/recover, `Game.State.Exhausted`
- Controle de sprint como input contextual de atributos:
  - `SetSprinting` e command `SetSprinting`
- APIs de mutacao de atributos:
  - `ConsumeStamina`, `AddStamina`, `ApplyDamage`, `ApplyHeal`
- Queries/eventos de atributos:
  - `IsExhausted`, `GetStamina`, `GetStateTagsCsv` (parte de estado de atributos), `Exhausted`, `ExhaustedCleared`
- Carregamento de recipe de atributos:
  - `TryLoadRecipeFromConfig`, `Omni.Attributes.Recipe`
  - hoje implementado dentro de `Status`
- Tipagem/config acoplada indevidamente a status:
  - `FOmniStatusSettings` em `OmniStatusData.h` (na pratica e config de stamina/attributes)

`SYS_Status` deve ficar apenas com:

- efeitos temporarios (`ApplyStatus`/`RemoveStatus`)
- duracao/stacks/tick deterministico
- snapshot de status ativo
- tags/efeitos de status (ex.: `Game.Status.*`) e bridge com modifiers

## 2) Quais arquivos/sistemas seriam impactados

- Nucleo de contrato/mensageria:
  - `Plugins/Omni/Source/OmniCore/Public/Systems/OmniSystemMessageSchemas.h`
  - `Plugins/Omni/Source/OmniCore/Private/Systems/OmniSystemMessageSchemas.cpp`
- Systems runtime:
  - `Plugins/Omni/Source/OmniRuntime/Public/Systems/Status/OmniStatusSystem.h`
  - `Plugins/Omni/Source/OmniRuntime/Private/Systems/Status/OmniStatusSystem.cpp`
  - novo `OmniAttributesSystem` em `Plugins/Omni/Source/OmniRuntime/Public|Private/Systems/Attributes/*`
- Manifest e dependencias:
  - `Plugins/Omni/Source/OmniRuntime/Private/Manifest/OmniOfficialManifest.cpp`
- Consumers diretos do `Status` atual:
  - `Plugins/Omni/Source/OmniRuntime/Private/Systems/Movement/OmniMovementSystem.cpp`
  - `Plugins/Omni/Source/OmniRuntime/Private/Systems/ActionGate/OmniActionGateSystem.cpp`
  - `Plugins/Omni/Source/OmniRuntime/Private/Systems/Camera/OmniCameraSystem.cpp`
  - `Plugins/Omni/Source/OmniRuntime/Private/Systems/UI/OmniUIBridgeSubsystem.cpp`
  - `Plugins/Omni/Source/OmniRuntime/Private/Systems/Animation/OmniAnimBridgeComponent.cpp`
  - `Plugins/Omni/Source/OmniRuntime/Private/OmniRuntimeModule.cpp`
  - `Plugins/Omni/Source/OmniExperimental/Private/Systems/MotionMatching/OmniMotionMatchingSubsystem.cpp`
- Config/data/governanca:
  - `Config/DefaultGame.ini`
  - `Scripts/omni_conformance_gate.ps1`
  - assets de status/attributes em `Content/Data/Status` (e possivel novo `Content/Data/Attributes`)

## 3) Estrategia de migracao em etapas

### Etapa A - Introduzir SYS_Attributes em paralelo (compat mode)

- Criar `UOmniAttributesSystem` com o comportamento atual de atributos (HP/Stamina/exhausted/recipe).
- Adicionar entry `Attributes` no manifest.
- Manter `Status` ainda respondendo contratos antigos de atributos via proxy/delegacao temporaria.
- Objetivo: subir os 2 systems sem quebrar runtime.

### Etapa B - Migrar contratos canonicos para Attributes

- Em schemas, introduzir `SystemAttributes` e mover `SetSprinting`, `IsExhausted`, eventos `Exhausted*` para `Attributes`.
- Manter aceitacao legada por 1 janela de transicao (validator aceitando old/new) com log de deprecacao.
- Atualizar consumers:
  - `Movement`: command/query/eventos com `Attributes`
  - `UIBridge`: snapshot de `Attributes`
  - `AnimBridge` e `MotionMatching`: `IsExhausted` de `Attributes`

### Etapa C - Purificar SYS_Status

- Remover de `Status` tudo que e atributo.
- `Status` fica apenas com status temporario (duration/stacks/tick/snapshot/tags/efeitos).
- Reforcar contrato de query de status tags (ex.: `GetStatusTagsCsv`), separado de estado de atributos.

### Etapa D - Ajustar agregacao de contexto para ActionGate/Camera

- `ActionGate` e `Camera` passam a compor contexto de 2 fontes:
  - atributos (ex.: `Game.State.Exhausted`, sprint state)
  - status temporario (`Game.Status.*`)
- Dependencias do manifest:
  - `Movement` -> `ActionGate + Attributes + Modifiers`
  - `ActionGate`/`Camera` -> `Status + Attributes`

### Etapa E - Canonizacao de data model e governanca

- Tirar `FOmniStatusSettings` do dominio de status (migrar para attributes).
- Atualizar conformance gate (deixar de validar modelo antigo de 3 profiles somente).
- Atualizar comandos console:
  - `omni.damage`/`omni.heal` -> `Attributes`
  - `omni.statusfx` permanece em `Status`
- Encerrar compat mode removendo aliases legados.

## 4) Riscos de regressao

- Sprint quebrar por mismatch de schema/target (`SetSprinting`, `IsExhausted`).
- ActionGate permissivo indevido se nao agregar tags de `Status` + `Attributes`.
- HUD/Anim/MM ficarem cegos (snapshot/exhausted apontando para system antigo).
- Eventos exhausted nao dispararem/serem ignorados por validacao de source system.
- Boot fail-fast no registry por dependencias/manifest inconsistentes.
- Conformance/CI quebrar por expectativa antiga de profile/assets.
- Drift de config/assets (recipe path legado em secao de `OmniStatusSystem`).
- Comportamento funcional mudar sem querer em `omni.sprint`, `omni.damage`, `omni.heal`, `omni.statusfx`.

## 5) Criterio de aceite

1. Separacao arquitetural comprovada
   - `SYS_Attributes` existe e concentra 100% de HP/Stamina/exhausted/recipe.
   - `SYS_Status` nao contem logica de atributo numerico.

2. Boot/runtime
   - Registry inicializa com systems separados (`Status` e `Attributes`) sem fallback DEV.
   - Manifest/dependencies coerentes.

3. Comportamento funcional preservado
   - Sprint drena stamina e exhausted interrompe sprint.
   - Regen com delay remove exhausted corretamente.
   - `omni.damage`/`omni.heal` funcionando via `Attributes`.
   - `omni.statusfx apply/remove/status` funcionando via `Status`.

4. Integracoes
   - ActionGate bloqueia corretamente por `Game.State.Exhausted` e por `Game.Status.*`.
   - Camera continua recebendo contexto correto.
   - HUD exibe snapshot correto.
   - AnimBridge/MM continuam refletindo exhausted.

5. Qualidade e governanca
   - Build OK.
   - Conformance gate atualizado e PASS.
   - Smoke headless minimo PASS para:
     - sprint/exhausted
     - damage/heal
     - statusfx stack/expire
     - animbridge status
     - camera status

