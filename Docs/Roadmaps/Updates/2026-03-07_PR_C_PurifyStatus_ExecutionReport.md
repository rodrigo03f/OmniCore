# PR C - Purificar SYS_Status (Execution Report)

## Escopo executado
PR C foi aplicado com escopo fechado em `SYS_Status`, sem refactor lateral.

## 1) O que foi removido de Status
APIs públicas de atributo removidas de `UOmniStatusSystem`:
- `GetSnapshot`
- `GetCurrentStamina`
- `GetMaxStamina`
- `GetStaminaNormalized`
- `IsExhausted`
- `SetSprinting`
- `ConsumeStamina`
- `AddStamina`
- `ApplyDamage`
- `ApplyHeal`

Handlers de contratos de atributo removidos no runtime de `Status`:
- `CommandSetSprinting`
- `CommandConsumeStamina`
- `CommandAddStamina`
- `QueryIsExhausted`
- `Query GetStamina`

Arquivos alterados:
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/Status/OmniStatusSystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/Status/OmniStatusSystem.cpp`

## 2) O que permaneceu em Status
Responsabilidades mantidas em `SYS_Status`:
- `ApplyStatus` / `RemoveStatus`
- duração, stacks, tick e expiração
- snapshot de efeitos (`GetStatusSnapshot`)
- bridge de modifiers por status
- query de contexto por tags (`GetStateTagsCsv`)

## 3) Resíduo explícito (compat temporária)
Para não quebrar consumidores legados antes do PR D:
- `GetStateTagsCsv` ainda agrega `Status + Attributes` via `RefreshCombinedStateTags`.

Também existem estruturas legadas fora do system com campos de stamina/exhausted:
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/Status/OmniStatusData.h`

## 4) Validação realizada
- Varredura estática confirma que `OmniStatusSystem` não contém mais lógica/API de:
  - `HP`
  - `Stamina`
  - `Damage/Heal`
  - `Exhausted`
- Neste passo não foi executado build/playtest (somente validação estática de código).
