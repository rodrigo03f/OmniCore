# Resumo — OmniPlaytestHostSubsystem (para explicação linha a linha)

## Arquivos
- `Source/OmniSandbox/OmniPlaytestHostSubsystem.h`
- `Source/OmniSandbox/OmniPlaytestHostSubsystem.cpp`

## Objetivo do subsystem
O `UOmniPlaytestHostSubsystem` cria automaticamente um host mínimo jogável no PIE para o pawn local:

1. garantir câmera funcional (`SpringArm + Camera`)
2. garantir controle (`WASD + mouse`) via Enhanced Input em runtime
3. manter compatível com o fluxo Omni já existente (`AvatarBridge`, `AnimBridge`, HUD, sprint no MovementSystem)

---

## Estrutura do `.h` (o contrato)

### Classe principal
- `UOmniPlaytestHostSubsystem : UGameInstanceSubsystem`
- É um subsystem de `GameInstance`, então sobe sem depender de blueprint manual extra.

### Ciclo de vida
- `Initialize(...)`
- `Deinitialize()`

### Funções de fluxo
- `Tick(float DeltaTime)`  
Loop principal para detectar controller/pawn local e manter o setup mínimo.

- `EnsureRuntimeInputAssets()`  
Cria `UInputAction` e `UInputMappingContext` em runtime.

- `HandlePawnChanged(...)`  
Reseta estado e reaplica setup quando muda o pawn possuído.

- `EnsurePlayableCamera(...)`  
Garante `SpringArm` + `CameraComponent`.

- `EnsurePlayableMovementDefaults(...)`  
Ajusta defaults mínimos no `CharacterMovement`.

- `EnsureInputModeGameOnly(...)`  
Força input mode de jogo.

- `EnsureRuntimeMappingApplied(...)`  
Aplica o Mapping Context no `EnhancedInputLocalPlayerSubsystem`.

- `EnsureRuntimeBindings(...)`  
Faz bind das actions para os handlers.

- `LogHostReady()`  
Log final de prontidão (camera/binding/bridges).

### Handlers de input
- `HandleMoveForward(...)`
- `HandleMoveRight(...)`
- `HandleLookYaw(...)`
- `HandleLookPitch(...)`

### Estado interno
- referências fracas do controller/pawn ativo
- ponteiro do `EnhancedInputComponent` bound
- `RuntimeMappingContext` e 4 actions (`MoveForward`, `MoveRight`, `LookYaw`, `LookPitch`)
- flags de controle (`bRuntimeMappingApplied`, `bRuntimeBindingsApplied`, `bLoggedHostReady`)

---

## Fluxo do `.cpp` (ordem real)

### 1) Startup
- `Initialize`:
  - chama `EnsureRuntimeInputAssets()`
  - registra ticker (`FTSTicker`) chamando `Tick`

### 2) Tick contínuo
No `Tick`:

1. resolve `GameInstance`
2. resolve `FirstLocalPlayerController`
3. resolve `PlayerController->GetPawn()`
4. se controller/pawn mudou: `HandlePawnChanged`
5. executa, em sequência:
   - `EnsurePlayableCamera`
   - `EnsurePlayableMovementDefaults`
   - `EnsureInputModeGameOnly`
   - `EnsureRuntimeMappingApplied`
   - `EnsureRuntimeBindings`
   - `LogHostReady`

### 3) Criação do input runtime
`EnsureRuntimeInputAssets`:

- cria context/action se não existirem
- define todas actions como `Axis1D`
- mapeia:
  - `W` = forward +
  - `S` = forward - (`UInputModifierNegate`)
  - `D` = right +
  - `A` = right - (`UInputModifierNegate`)
  - `MouseX` = yaw
  - `MouseY` = pitch invertido (`UInputModifierNegate`)

### 4) Troca de pawn
`HandlePawnChanged`:

- remove mapping do controller anterior
- reseta estado de binding
- salva novo `ActiveController/ActivePawn`
- reaplica setup mínimo imediato
- loga troca

### 5) Câmera mínima
`EnsurePlayableCamera`:

- procura `USpringArmComponent`; se não tiver, cria e registra
- aplica defaults (`arm length`, `offset`, rotação por controle, sem lag)
- procura `UCameraComponent`; se não tiver, cria e registra
- prende câmera no spring arm (socket) e deixa `bUsePawnControlRotation=false` na câmera

### 6) Movimento mínimo
`EnsurePlayableMovementDefaults`:

- só atua se pawn for `ACharacter`
- desliga `bUseControllerRotationYaw/Pitch/Roll`
- garante `CharacterMovement` válido
- define `MaxWalkSpeed=600` se estava zero
- ativa `bOrientRotationToMovement=true`
- aplica `RotationRate=(0,720,0)`

### 7) Input mode
`EnsureInputModeGameOnly`:

- `Controller->SetInputMode(FInputModeGameOnly{})`
- esconde cursor

### 8) Aplicação do Mapping Context
`EnsureRuntimeMappingApplied`:

- pega `ULocalPlayer`
- pega `UEnhancedInputLocalPlayerSubsystem`
- remove contexto (idempotência) e adiciona com prioridade `100`
- marca `bRuntimeMappingApplied=true`
- loga

### 9) Bind das ações
`EnsureRuntimeBindings`:

- pega `Controller->InputComponent` como `UEnhancedInputComponent`
- evita rebinding se já está no mesmo componente
- binda `Triggered` para:
  - forward
  - right
  - look yaw
  - look pitch
- marca `bRuntimeBindingsApplied=true`
- loga

### 10) Handlers
- `HandleMoveForward/Right`:
  - lê eixo float
  - pega `ControlRotation`, zera pitch/roll
  - converte em vetor direção via `FRotationMatrix`
  - chama `AddMovementInput`

- `HandleLookYaw/Pitch`:
  - lê eixo float
  - chama `AddControllerYawInput` / `AddControllerPitchInput`

### 11) Log de pronto
`LogHostReady` (uma vez):

- varre componentes do pawn
- detecta presença de `OmniAvatarBridgeComponent` e `OmniAnimBridgeComponent`
- loga:
  - spring arm
  - camera
  - bindings
  - avatar bridge
  - anim bridge

### 12) Shutdown
`Deinitialize`:

- remove mapping do controller ativo
- limpa binding state
- remove ticker
- limpa referências runtime

---

## Conclusão funcional
Esse subsystem não substitui systems Omni.  
Ele só monta a camada de host jogável que faltava (input + câmera + defaults de movimento), permitindo validar Omni em PIE com personagem controlável.
