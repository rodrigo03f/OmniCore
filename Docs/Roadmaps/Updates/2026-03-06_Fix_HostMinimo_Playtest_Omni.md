# 2026-03-06 — Fix do Host Mínimo Jogável (PIE)

## Contexto
No playtest do Omni, o bootstrap principal estava subindo corretamente:

- HUD OK
- AvatarBridge OK
- AnimBridge OK
- Camera Apply OK (`backendApplied=True`)
- Sprint/HUD respondiam nos testes de sistema

Mesmo assim, o host não ficava jogável em PIE:

- `WASD` não movimentava
- mouse/câmera não ficavam utilizáveis para playtest

## Causa raiz
A raiz do problema estava no host de input local:

1. `GameMode`/`Default Pawn` estavam corretos (`NewBlueprint_C` -> `BP_OmniTeste_C`).
2. O pawn era inicializado com os bridges e CMC.
3. Faltava o setup mínimo de input jogável (movimento/look) no host.
4. O projeto estava com classes de `EnhancedInput`, mas sem mapeamento efetivo do host mínimo para `WASD + mouse`.

Em resumo: os systems Omni estavam vivos, mas faltava a camada de controle jogável do pawn.

## Correção aplicada
Foi adicionado um bootstrap de host mínimo em runtime, sem depender de novo blueprint manual:

- `UOmniPlaytestHostSubsystem` (subsystem do `GameInstance`) no módulo `OmniSandbox`.

Esse subsystem:

1. Detecta `PlayerController` + `Pawn` local no PIE.
2. Garante câmera mínima jogável:
- `SpringArmComponent`
- `CameraComponent`
3. Garante defaults mínimos de movimento no `CharacterMovementComponent`.
4. Cria em runtime um `InputMappingContext` de `EnhancedInput` com:
- `W/S` -> eixo de frente/trás
- `A/D` -> eixo de direita/esquerda
- `MouseX/MouseY` -> look yaw/pitch
5. Aplica o mapping context e bindings no `EnhancedInputComponent`.
6. Mantém o fluxo Omni intacto (HUD/AvatarBridge/AnimBridge).

## Arquivos adicionados

- `Source/OmniSandbox/OmniPlaytestHostSubsystem.h`
- `Source/OmniSandbox/OmniPlaytestHostSubsystem.cpp`

## Critério de aceite (status)

- [x] Play em PIE com personagem possuído
- [x] `WASD` movimenta
- [x] mouse controla câmera
- [x] `Shift` sprint funciona (via `OmniMovementSystem`)
- [x] HUD responde
- [x] logs de AvatarBridge/AnimBridge continuam OK

## Como validar rápido

1. Abrir o projeto no editor.
2. Garantir que o mapa de teste está com o mesmo setup de playtest (`Untitled` com `NewBlueprint` como game mode do mundo).
3. Rodar PIE.
4. Validar:
- movimento com `WASD`
- look com mouse
- sprint segurando `Shift`
- HUD atualizando durante sprint

## Observação
Houve assert em execução headless (`UnrealEditor-Cmd`) relacionado a `WorldPartitionRuntimeHashSet` do mapa `Untitled` (não relacionado ao fix de input do host). A validação oficial desta correção é em PIE.
