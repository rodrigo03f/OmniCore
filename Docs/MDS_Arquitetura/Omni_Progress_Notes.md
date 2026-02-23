# Omni Progress Notes

Este arquivo registra a evolucao tecnica do projeto Omni de forma didatica.
Objetivo: facilitar manutencao e aprendizado sem depender de memoria.

## Como registrar cada etapa

Para cada entrega nova, preencher:

1. Objetivo da etapa
2. Onde encaixa na arquitetura (Core/Runtime/GAS/Editor/Forge)
3. Arquivos criados/alterados
4. Como funciona (fluxo curto)
5. Como testar
6. Riscos/observacoes

---

## Etapa 01 - Fundacao do Plugin Omni

### Objetivo
Criar base modular do plugin com separacao correta entre runtime e editor.

### Encaixe na arquitetura
- `OmniCore`: contratos e tipos base
- `OmniRuntime`: execucao de sistemas
- `OmniGAS`: integracao com GAS
- `OmniEditor`: ferramentas editoriais
- `OmniForge`: pipeline de geracao

### Arquivos principais
- `Plugins/Omni/Omni.uplugin`
- `Plugins/Omni/Source/OmniCore/*`
- `Plugins/Omni/Source/OmniRuntime/*`
- `Plugins/Omni/Source/OmniGAS/*`
- `Plugins/Omni/Source/OmniEditor/*`
- `Plugins/Omni/Source/OmniForge/*`
- `OmniSandbox.uproject` (plugin habilitado)

### Fluxo
Editor carrega o plugin -> modulos sao carregados por tipo -> runtime/editor ficam desacoplados.

### Teste rapido
- Abrir projeto e compilar.
- Verificar plugin Omni habilitado.

### Observacoes
- Nucleo nao usa GameFeature plugin nesta fase.

---

## Etapa 02 - Debug Overlay v1 (leve)

### Objetivo
Ter observabilidade em runtime com baixo custo.

### Encaixe na arquitetura
- `OmniRuntime`: coleta e exibe dados de debug.

### Arquivos principais
- `Plugins/Omni/Source/OmniRuntime/Public/Debug/OmniDebugTypes.h`
- `Plugins/Omni/Source/OmniRuntime/Public/Debug/OmniDebugSubsystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Debug/OmniDebugSubsystem.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Debug/UI/OmniDebugOverlayWidget.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Debug/UI/OmniDebugOverlayWidget.cpp`
- `Plugins/Omni/Source/OmniRuntime/Private/OmniRuntimeModule.cpp`

### Fluxo
Sistemas registram eventos -> `UOmniDebugSubsystem` guarda em buffer -> widget mostra snapshot em pulso curto.

### Teste rapido
- `F10`: abre/fecha overlay
- `F9`: limpa entradas
- `omni.debug 0|1|2`
- `omni.debug.toggle`
- `omni.debug.clear`

### Observacoes
- Texto do overlay foi ajustado para pt-BR.
- UI usa atualizacao por revisao para reduzir custo.

---

## Etapa 03 - Localizacao inicial

### Objetivo
Preparar UI para multiplos idiomas desde o inicio.

### Encaixe na arquitetura
- `OmniRuntime/UI`: textos via `FText` + `LOCTEXT`.
- Packaging: culturas staged no projeto.

### Arquivos principais
- `Plugins/Omni/Source/OmniRuntime/Private/Debug/UI/OmniDebugOverlayWidget.cpp`
- `Config/DefaultGame.ini`
- `Omni_Localization_Quickstart.md`

### Fluxo
Textos fixos em `LOCTEXT` -> Gather/Compile no Localization Dashboard -> engine aplica cultura ativa.

### Teste rapido
- Alterar cultura de preview no editor e abrir overlay.

### Observacoes
- Culturas staged: `en`, `pt-BR`, `es`, `fr`, `de`, `ja`.

---

## Etapa 04 - Contratos de Sistema + SystemRegistry (inicio P0)

### Objetivo
Iniciar infraestrutura deterministica de sistemas orientados por Manifest.

### Encaixe na arquitetura
- `OmniCore`: contrato de sistema e manifesto.
- `OmniRuntime`: registry que instancia e ordena systems por dependencia.

### Arquivos principais
- `Plugins/Omni/Source/OmniCore/Public/Systems/OmniRuntimeSystem.h`
- `Plugins/Omni/Source/OmniCore/Private/Systems/OmniRuntimeSystem.cpp`
- `Plugins/Omni/Source/OmniCore/Public/Manifest/OmniManifest.h`
- `Plugins/Omni/Source/OmniCore/Private/Manifest/OmniManifest.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/OmniSystemRegistrySubsystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/OmniSystemRegistrySubsystem.cpp`

### Fluxo
Manifest define systems -> Registry resolve classes e dependencias -> ordena de forma deterministica -> inicializa systems -> tick opcional.

### Teste rapido
- Build limpo (evitar live coding apos novas UCLASS/USTRUCT).
- Inicializar registry com um manifest simples e validar logs.

### Observacoes
- Registry valida duplicata de ID.
- Registry detecta ciclo de dependencia.

---

## Etapa 05 - Organizacao por pasta de sistema (stubs P0)

### Objetivo
Padronizar estrutura por sistema e iniciar classes concretas para ActionGate, Movement e Status.

### Encaixe na arquitetura
- `OmniRuntime`: systems concretos herdando `UOmniRuntimeSystem`.

### Convencao de pasta
- Public:
  - `Plugins/Omni/Source/OmniRuntime/Public/Systems/ActionGate/*`
  - `Plugins/Omni/Source/OmniRuntime/Public/Systems/Movement/*`
  - `Plugins/Omni/Source/OmniRuntime/Public/Systems/Status/*`
- Private:
  - `Plugins/Omni/Source/OmniRuntime/Private/Systems/ActionGate/*`
  - `Plugins/Omni/Source/OmniRuntime/Private/Systems/Movement/*`
  - `Plugins/Omni/Source/OmniRuntime/Private/Systems/Status/*`

### Arquivos principais
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/ActionGate/OmniActionGateSystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/ActionGate/OmniActionGateSystem.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/Movement/OmniMovementSystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/Movement/OmniMovementSystem.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/Status/OmniStatusSystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/Status/OmniStatusSystem.cpp`

### Fluxo
Registry le manifest -> identifica classes dos systems -> instancia em ordem de dependencia.
Dependencias iniciais:
- `Movement` -> `ActionGate`
- `Status` -> `ActionGate`

### Teste rapido
- Build limpo.
- Criar manifest com esses systems e validar logs de inicializacao.

### Observacoes
- Nesta etapa, os systems estao em modo stub (base pronta, sem regra de gameplay completa).

---

## Etapa 06 - Bootstrap automatico do Registry (P0)

### Objetivo
Garantir inicializacao automatica dos systems mesmo sem manifest asset pronto.

### Encaixe na arquitetura
- `OmniRuntime`: bootstrap no `UOmniSystemRegistrySubsystem`.
- `Config`: declaracao de classes fallback no `DefaultGame.ini`.

### Arquivos principais
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/OmniSystemRegistrySubsystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/OmniSystemRegistrySubsystem.cpp`
- `Config/DefaultGame.ini`

### Fluxo
Na inicializacao do `GameInstanceSubsystem`:
1. tenta `AutoManifestAssetPath` (quando existir um `UOmniManifest` autoritativo);
2. se nao houver/nao carregar, usa fallback configurado (`FallbackSystemClasses`);
3. monta manifest transiente e passa pelo mesmo pipeline de validacao/ordem/dependencia.

### Teste rapido
- Build limpo.
- Abrir PIE sem configurar manifest asset.
- Validar logs de `Registry inicializado via fallback` e inicializacao de `ActionGate`, `Movement`, `Status`.

### Observacoes
- Mantem a arquitetura orientada a manifest, mas sem bloquear prototipo por falta de asset.
- Em producao, o ideal e migrar para `AutoManifestAssetPath` dedicado.

---

## Etapa 07 - ActionGate v1 + Sprint Smoke Loop (P0)

### Objetivo
Fechar o primeiro loop jogavel de sistemas com regra real de autorizacao de acao.

### Encaixe na arquitetura
- `ActionGate`: `BlockedBy`, `Cancels`, `AppliesLocks`, `Policy`.
- `Status`: stamina, regen, estado `Exhausted`.
- `Movement`: solicita sprint via gate e reage a exaustao.
- `Debug`: telemetria em tempo real no overlay.
- `Registry`: manifest oficial por classe.

### Arquivos principais
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/ActionGate/OmniActionGateTypes.h`
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/ActionGate/OmniActionGateSystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/ActionGate/OmniActionGateSystem.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/Status/OmniStatusSystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/Status/OmniStatusSystem.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/Movement/OmniMovementSystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/Movement/OmniMovementSystem.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Manifest/OmniOfficialManifest.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Manifest/OmniOfficialManifest.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Debug/OmniDebugSubsystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Debug/OmniDebugSubsystem.cpp`
- `Plugins/Omni/Source/OmniRuntime/Private/Debug/UI/OmniDebugOverlayWidget.cpp`
- `Plugins/Omni/Source/OmniRuntime/Public/Systems/OmniSystemRegistrySubsystem.h`
- `Plugins/Omni/Source/OmniRuntime/Private/Systems/OmniSystemRegistrySubsystem.cpp`
- `Plugins/Omni/Source/OmniRuntime/Private/OmniRuntimeModule.cpp`
- `Config/DefaultGame.ini`
- `Config/DefaultGameplayTags.ini`

### Fluxo
1. Registry carrega `UOmniOfficialManifest` (config por `AutoManifestClassPath`).
2. Inicializa `Status -> ActionGate -> Movement` em ordem deterministica.
3. `Movement` tenta iniciar `Movement.Sprint` no `ActionGate`.
4. `ActionGate` valida `BlockedBy` (`State.Exhausted`), aplica locks e retorna decisao.
5. `Status` drena/recupera stamina e marca/limpa `Exhausted`.
6. Overlay mostra metricas em tempo real (stamina, sprint, locks, ultima decisao do gate).

### Teste rapido
- Abrir PIE e exibir overlay (`F10`).
- Console: `omni.sprint auto 12`
- Console: `omni.sprint status`
- Esperado:
  - stamina cai, entra em `Exhausted`, sprint para;
  - durante `Exhausted`, gate nega sprint;
  - ao recuperar acima do threshold, sprint pode voltar.

### Observacoes
- `omni.sprint` suporta: `start`, `stop`, `toggle`, `auto <segundos>`, `status`.
- Para producao final, ainda vale criar um `UOmniManifest` asset de conteudo se quiser pipeline 100% data-driven no Content Browser.

---

## Automacoes de suporte

Arquivos:
- `Scripts/watch_build_errors.ps1`
- `Scripts/collect_omni_diagnostics.ps1`
- `Scripts/check_debug_overlay_signals.ps1`
- `Scripts/README.md`

Objetivo:
acelerar diagnostico de build/crash e verificacao do overlay.
