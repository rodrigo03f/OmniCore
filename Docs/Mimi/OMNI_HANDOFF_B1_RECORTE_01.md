# OMNI - HANDOFF B1 (RECORTE 01)

## 1) Estado de commit
- Commit realizado: **nao**
- Branch atual: `main`
- Link de commit: **N/A**

## 2) Objetivo do recorte
- Iniciar Sprint B1 migrando o `ActionGate` de hardcode para fonte de dados via `Manifest -> Profile/Library`.
- Manter fallback em C++ apenas como `DEV_FALLBACK` com log explicito.

## 3) O que foi implementado

### 3.1 Manifest com settings por system
- `FOmniSystemManifestEntry` agora tem:
  - `TMap<FName, FString> Settings`
- Arquivo:
  - `Plugins/Omni/Source/OmniCore/Public/Manifest/OmniManifest.h`

### 3.2 Contratos novos de dados (Library/Profile)
- Criado `UOmniActionLibrary` (lista de `FOmniActionDefinition`).
- Criado `UOmniDevActionLibrary` (seed DEV com `Movement.Sprint`).
- Criado `UOmniActionProfile`:
  - referencia opcional para `ActionLibrary`
  - `OverrideDefinitions`
  - `ResolveDefinitions(...)`
- Criado `UOmniDevActionProfile`:
  - resolve dados com base no `UOmniDevActionLibrary`.
- Arquivos:
  - `Plugins/Omni/Source/OmniRuntime/Public/Library/OmniActionLibrary.h`
  - `Plugins/Omni/Source/OmniRuntime/Private/Library/OmniActionLibrary.cpp`
  - `Plugins/Omni/Source/OmniRuntime/Public/Profile/OmniActionProfile.h`
  - `Plugins/Omni/Source/OmniRuntime/Private/Profile/OmniActionProfile.cpp`

### 3.3 ActionGate data-driven com fallback DEV
- `UOmniActionGateSystem` agora:
  - tenta carregar definitions via `Manifest` usando:
    - `ActionProfileAssetPath`
    - `ActionProfileClassPath`
  - resolve via `UOmniActionProfile::ResolveDefinitions(...)`
  - se falhar, usa `DEV_FALLBACK` com log explicito
  - se necessario, fallback final inline C++ (ultimo recurso)
- Ajuste de robustez:
  - lookup do entry no manifest por `SystemId` e fallback por `SystemClass`.
- Arquivos:
  - `Plugins/Omni/Source/OmniRuntime/Public/Systems/ActionGate/OmniActionGateSystem.h`
  - `Plugins/Omni/Source/OmniRuntime/Private/Systems/ActionGate/OmniActionGateSystem.cpp`

### 3.4 OfficialManifest apontando profile DEV
- Entry do `ActionGate` passou a declarar:
  - `Settings["ActionProfileClassPath"] = "/Script/OmniRuntime.OmniDevActionProfile"`
- Arquivo:
  - `Plugins/Omni/Source/OmniRuntime/Private/Manifest/OmniOfficialManifest.cpp`

### 3.5 Logs explicitos de DEV_FALLBACK no Registry
- `UOmniSystemRegistrySubsystem` agora loga warning claro quando inicializa por fallback DEV:
  - `AutoManifestClassPath`
  - `FallbackSystemClasses` de config
- Arquivo:
  - `Plugins/Omni/Source/OmniRuntime/Private/Systems/OmniSystemRegistrySubsystem.cpp`

## 4) Arquivos alterados (tracked)
- `Plugins/Omni/Source/OmniCore/Public/Manifest/OmniManifest.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Manifest/OmniOfficialManifest.cpp`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/ActionGate/OmniActionGateSystem.cpp`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/OmniSystemRegistrySubsystem.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/ActionGate/OmniActionGateSystem.h`

## 5) Arquivos novos (untracked no momento)
- `Plugins/Omni/Source/OmniRuntime/Public/Library/OmniActionLibrary.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Library/OmniActionLibrary.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Profile/OmniActionProfile.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Profile/OmniActionProfile.cpp`

## 6) Validacao executada
- Comando:
  - `powershell -ExecutionPolicy Bypass -File .\Scripts\clean_rebuild_normal.ps1 -KillUnrealEditor`
- Resultado:
  - **Succeeded** (clean rebuild completo)
- Comando adicional:
  - `Build.bat OmniSandboxEditor Win64 Development ...`
- Resultado:
  - **Succeeded** (incremental)

## 7) Proximo passo recomendado (B1 recorte 02)
- Criar assets reais de `UOmniActionLibrary/UOmniActionProfile` no Content e migrar de `ActionProfileClassPath` para `ActionProfileAssetPath`.
- Aplicar o mesmo padrao `Manifest -> Profile/Library` para `Status` e `Movement`.
- Definir criterio de aceite: sem regra hardcoded nos systems (exceto caminho `DEV_FALLBACK` com log explicito).
