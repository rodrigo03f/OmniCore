# OmniSandbox - Mapa Explicado dos Arquivos de Codigo (sem Docs)

> Escopo: este documento cobre os arquivos de codigo e configuracao do projeto fora de `Docs/`.
> Inclui: `Source/`, `Plugins/Omni/Source/`, `Scripts/`, `Config/` e metadados de projeto.
> Nao inclui analise linha-a-linha do plugin de terceiro `Plugins/Developer/RiderLink` (vendor/IDE integration).

## 1. Visao geral arquitetural

- O projeto Unreal tem um modulo de jogo base (`OmniSandbox`) e um plugin principal (`Omni`) com 5 modulos:
  - `OmniCore` (contratos base: manifest, runtime system abstrato, mensagens e schemas)
  - `OmniRuntime` (systems reais: Registry, Clock, Status, ActionGate, Movement, Debug)
  - `OmniForge` (pipeline de normalizacao/validacao/resolve/geracao de artefatos)
  - `OmniEditor` (casca editor)
  - `OmniGAS` (casca runtime para integracao futura com GAS)
- Fluxo operacional principal:
  - `Registry` inicializa systems via `Manifest` + ordem topologica
  - `Movement` consulta `ActionGate` e sincroniza estado com `Status`
  - `Status` calcula stamina/exhausted e publica eventos
  - `ActionGate` decide allow/deny/start/stop de acoes via definicoes data-driven
  - `DebugSubsystem` agrega logs/metricas e opcionalmente mostra overlay

## 2. Arquivos raiz do projeto

### `OmniSandbox.uproject`
- Faz:
  - Declara o projeto UE 5.7.
  - Ativa plugin `Omni`.
  - Define modulo principal `OmniSandbox` (runtime).
- Nao faz:
  - Nao define logica de gameplay.
  - Nao define pipeline de systems (isso esta no plugin `Omni` + config).

### `OmniSandbox.sln`
- Faz:
  - Solucao Visual Studio gerada pelo Unreal Build Tool.
- Nao faz:
  - Nao contem regra de negocio; e infraestrutura de build/IDE.

## 3. Modulo base do jogo (`Source/OmniSandbox`)

### `Source/OmniSandbox.Target.cs`
- Faz:
  - Define target de jogo (`TargetType.Game`) e modulo extra `OmniSandbox`.
- Nao faz:
  - Nao compila logica por si.

### `Source/OmniSandboxEditor.Target.cs`
- Faz:
  - Define target de editor (`TargetType.Editor`) para o projeto.
- Nao faz:
  - Nao injeta comportamento de runtime.

### `Source/OmniSandbox/OmniSandbox.Build.cs`
- Faz:
  - Define dependencias do modulo base (`Core`, `Engine`, `EnhancedInput`, etc.).
- Nao faz:
  - Nao implementa features de gameplay.

### `Source/OmniSandbox/OmniSandbox.h`
- Faz:
  - Header minimo do modulo de jogo.
- Nao faz:
  - Nao declara classes/systems de gameplay.

### `Source/OmniSandbox/OmniSandbox.cpp`
- Faz:
  - Registra o modulo principal via `IMPLEMENT_PRIMARY_GAME_MODULE`.
- Nao faz:
  - Nao inicializa sistemas Omni manualmente.

## 4. Plugin Omni (descricao por modulo)

### `Plugins/Omni/Omni.uplugin`
- Faz:
  - Declara modulos `OmniCore`, `OmniRuntime`, `OmniGAS`, `OmniEditor`, `OmniForge`.
- Nao faz:
  - Nao implementa regras de runtime; so metadata de carregamento.

### 4.1 OmniCore

#### `Plugins/Omni/Source/OmniCore/OmniCore.Build.cs`
- Faz:
  - Dependencias base (`GameplayTags`, etc.).
- Nao faz:
  - Nao executa logica.

#### `Plugins/Omni/Source/OmniCore/Public/OmniCoreModule.h`
#### `Plugins/Omni/Source/OmniCore/Private/OmniCoreModule.cpp`
- Faz:
  - Casca de modulo Unreal (`StartupModule`/`ShutdownModule` vazios).
- Nao faz:
  - Nao possui inicializacao de systems.

#### `Plugins/Omni/Source/OmniCore/Public/Manifest/OmniManifest.h`
#### `Plugins/Omni/Source/OmniCore/Private/Manifest/OmniManifest.cpp`
- Faz:
  - Define `UOmniManifest` e `FOmniSystemManifestEntry`.
  - Fornece mapa de `Settings` por system (`SetSetting`, `TryGetSetting`, snapshot de chaves ordenado).
  - Permite localizar entry por `SystemId`.
- Nao faz:
  - Nao instancia systems.
  - Nao valida dependencia circular.

#### `Plugins/Omni/Source/OmniCore/Public/Systems/OmniRuntimeSystem.h`
#### `Plugins/Omni/Source/OmniCore/Private/Systems/OmniRuntimeSystem.cpp`
- Faz:
  - Classe abstrata base para todos os systems (`Initialize`, `Tick`, `HandleCommand/Query/Event`).
  - `IsInitializationSuccessful` + controle de resultado de init.
  - Implementacoes default no-op (seguras).
- Nao faz:
  - Nao define regras de negocio de nenhum system.

#### `Plugins/Omni/Source/OmniCore/Public/Systems/OmniSystemMessaging.h`
- Faz:
  - Contratos de mensagem (`FOmniCommandMessage`, `FOmniQueryMessage`, `FOmniEventMessage`).
  - Helpers de leitura/escrita de argumentos/output/payload.
- Nao faz:
  - Nao roteia mensagem entre systems (isso e do Registry).
  - Nao valida schema semanticamente.

#### `Plugins/Omni/Source/OmniCore/Public/Systems/OmniSystemMessageSchemas.h`
#### `Plugins/Omni/Source/OmniCore/Private/Systems/OmniSystemMessageSchemas.cpp`
- Faz:
  - Define nomes canonicos de systems/commands/queries/events.
  - Implementa schema tipado para StartAction, StopAction, SetSprinting, CanStartAction, IsExhausted, etc.
  - Converte struct <-> message e valida payload.
  - `FOmniMessageSchemaValidator` central para command/query/event.
- Nao faz:
  - Nao executa a acao de gameplay; so valida/transcodifica mensagens.

### 4.2 OmniRuntime

#### `Plugins/Omni/Source/OmniRuntime/OmniRuntime.Build.cs`
- Faz:
  - Dependencias de runtime e UI (`UMG`, `Slate`, `OmniCore`).
- Nao faz:
  - Nao contem logica de system.

#### `Plugins/Omni/Source/OmniRuntime/Public/OmniRuntimeModule.h`
#### `Plugins/Omni/Source/OmniRuntime/Private/OmniRuntimeModule.cpp`
- Faz:
  - Registra modulo runtime.
  - Faz `TagGuard` no startup para proteger namespace reservado `Omni.*`.
  - Registra comandos de console:
    - `omni.debug.toggle`
    - `omni.debug.clear`
    - `omni.sprint start|stop|toggle|auto|status`
  - Define `omni.debug` CVar.
- Nao faz:
  - Nao implementa a mecanica de sprint diretamente; apenas chama systems via registry/subsystems.
  - Nao permite tag `Omni.*` arbitraria (falha fatal se ilegal).

#### `Plugins/Omni/Source/OmniRuntime/Public/Manifest/OmniOfficialManifest.h`
#### `Plugins/Omni/Source/OmniRuntime/Private/Manifest/OmniOfficialManifest.cpp`
- Faz:
  - Monta manifest oficial default com `Status`, `ActionGate`, `Movement`.
  - Define dependencias e paths default de profile asset para cada system.
- Nao faz:
  - Nao resolve nem valida assets em runtime por si (cada system faz sua validacao).

#### Libraries (`Public/Library/*.h`, `Private/Library/*.cpp`)
- `OmniActionLibrary.h/.cpp`
  - Faz: data asset de definicoes de acao; versao dev cria acao `Movement.Sprint` com `State.Exhausted` como bloqueio e lock `Lock.Movement.Sprint`.
  - Nao faz: nao executa acao.
- `OmniStatusLibrary.h/.cpp`
  - Faz: data asset de `FOmniStatusSettings`; dev library garante tag exhausted default quando possivel.
  - Nao faz: nao drena/regen stamina.
- `OmniMovementLibrary.h/.cpp`
  - Faz: data asset de `FOmniMovementSettings`; dev library garante `SprintActionId`.
  - Nao faz: nao movimenta personagem.

#### Profiles (`Public/Profile/*.h`, `Private/Profile/*.cpp`)
- `OmniActionProfile.h/.cpp`
  - Faz: combina `ActionLibrary` + `OverrideDefinitions`.
  - Nao faz: nao valida runtime rules profundas (isso ocorre no `ActionGateSystem`).
- `OmniStatusProfile.h/.cpp`
  - Faz: resolve settings de status por library/override e valida `FOmniStatusSettings`.
  - Nao faz: nao aplica tick/status.
- `OmniMovementProfile.h/.cpp`
  - Faz: resolve settings de movement por library/override e valida `FOmniMovementSettings`.
  - Nao faz: nao controla sprint.

#### Debug Types/UI

##### `Public/Debug/OmniDebugTypes.h`
- Faz:
  - Tipos de nivel de log, modo debug, entrada e metrica.
- Nao faz:
  - Nao coleta dados sozinho.

##### `Public/Debug/OmniDebugSubsystem.h`
##### `Private/Debug/OmniDebugSubsystem.cpp`
- Faz:
  - Subsystem de debug com entries + metricas + revisao.
  - Suporta `Off/Basic/Full`.
  - Sincroniza modo com CVar `omni.debug`.
  - Atalhos F10 (toggle overlay) e F9 (clear entries).
  - Controla widget overlay.
- Nao faz:
  - Nao substitui logging de engine.
  - Nao persiste logs em disco diretamente.

##### `Public/Debug/UI/OmniDebugOverlayWidget.h`
##### `Private/Debug/UI/OmniDebugOverlayWidget.cpp`
- Faz:
  - Widget UMG para mostrar metricas e logs recentes do debug subsystem.
  - Construcao de UI basica em runtime (header/body).
  - Refresh periodico por timer.
- Nao faz:
  - Nao calcula metricas; so apresenta.
  - Nao escreve de volta no subsystem.

#### Clock

##### `Public/Systems/OmniClockSubsystem.h`
##### `Private/Systems/OmniClockSubsystem.cpp`
- Faz:
  - Fonte unica de tempo simulado (`SimTimeSeconds`) e tick index.
  - Incrementa no tick com clamp de delta >= 0.
  - Permite reset.
- Nao faz:
  - Nao sincroniza tempo de rede.
  - Nao depende de wall-clock.

#### Registry

##### `Public/Systems/OmniSystemRegistrySubsystem.h`
##### `Private/Systems/OmniSystemRegistrySubsystem.cpp`
- Faz:
  - Bootstrap do runtime via manifest (asset/class configurado).
  - Resolve specs de systems, valida classes e `SystemId`.
  - Ordena inicializacao por dependencias (topological sort deterministico).
  - Inicializa systems e interrompe em fail-fast quando init falha.
  - Roteia `DispatchCommand`, `ExecuteQuery`, `BroadcastEvent` com validacao de schema.
  - Suporta modo DEV opt-in (`omni.devdefaults`) e fallback estrutural por config.
  - Publica metricas de diagnostico (`Omni.ManifestLoaded`, `Omni.DevDefaults`).
- Nao faz:
  - Nao permite fallback estrutural em modo normal sem opt-in DEV.
  - Nao tolera ciclo de dependencias.
  - Nao ignora payload invalido (rejeita e loga).

#### ActionGate

##### `Public/Systems/ActionGate/OmniActionGateTypes.h`
- Faz:
  - Tipos de policy (`DenyIfActive`, `SucceedIfActive`, `RestartIfActive`), definicao e decisao.
- Nao faz:
  - Nao aplica regras sozinho.

##### `Public/Systems/ActionGate/OmniActionGateSystem.h`
##### `Private/Systems/ActionGate/OmniActionGateSystem.cpp`
- Faz:
  - Carrega `ActionProfile` via manifest e valida definitions.
  - Modo strict/non-strict controlado por `omni.actiongate.failfast`.
  - Regras de start:
    - action existe e habilitada
    - policy para action ja ativa
    - bloqueio por tags (`Status` + locks ativos)
    - cancela acoes especificadas em `Cancels`
    - aplica/remocao de locks por refcount
  - Exponde consultas: active actions, active locks, known action ids, last decision.
  - Publica eventos lifecycle: started/ended/denied + action allowed/denied.
  - Mantem ordenacao estavel para exposicao publica.
- Nao faz:
  - Nao faz tick continuo (`IsTickEnabled=false`).
  - Nao usa `Input.*` como ActionId valido.
  - Nao depende de chamada direta de system (usa registry/messages).

#### Movement

##### `Public/Systems/Movement/OmniMovementData.h`
- Faz:
  - Define `FOmniMovementSettings` e validacao basica.
- Nao faz:
  - Nao altera estado de sprint.

##### `Public/Systems/Movement/OmniMovementSystem.h`
##### `Private/Systems/Movement/OmniMovementSystem.cpp`
- Faz:
  - Carrega movement profile via manifest.
  - Exige `OmniClockSubsystem` para operar.
  - Controla estado `SprintRequested`/`IsSprinting`.
  - Opcionalmente usa teclado Shift como input de request.
  - `StartAutoSprint` por duracao.
  - Consulta `ActionGate` (`CanStartAction`) e despacha start/stop action.
  - Sincroniza `Status` com `SetSprinting`.
  - Interrompe sprint se `Status` reportar exhausted (query ou event).
  - Publica metricas de telemetria.
  - Fallback de profile class apenas em DEV (`omni.devdefaults` + `omni.movement.devfallback`).
- Nao faz:
  - Nao altera velocidade fisica do personagem diretamente.
  - Nao ignora negacao do gate.
  - Nao opera sem clock (fail-fast).

#### Status

##### `Public/Systems/Status/OmniStatusData.h`
- Faz:
  - Define `FOmniStatusSettings` (max/drain/regen/delays/thresholds/tag) + validacao.
- Nao faz:
  - Nao aplica update temporal.

##### `Public/Systems/Status/OmniStatusSystem.h`
##### `Private/Systems/Status/OmniStatusSystem.cpp`
- Faz:
  - Carrega status profile via manifest.
  - Mantem stamina atual, sprinting, exhausted e state tags.
  - Tick:
    - drena stamina quando sprinting
    - regenera apos delay quando nao sprinting
    - detecta entrada/saida de exhausted por thresholds
    - publica eventos exhausted/exhaustedCleared
  - Commands:
    - `SetSprinting`
    - `ConsumeStamina`
    - `AddStamina`
  - Queries:
    - `IsExhausted`
    - `GetStateTagsCsv`
    - `GetStamina` (com output current/max/normalized)
  - Publica metricas de telemetria.
- Nao faz:
  - Nao consome input.
  - Nao decide autorizacao de acao (isso e do ActionGate).

### 4.3 OmniForge

#### `Plugins/Omni/Source/OmniForge/OmniForge.Build.cs`
- Faz:
  - Dependencias editor e suporte a Json/AssetTools/Projects.
- Nao faz:
  - Nao executa pipeline.

#### `Plugins/Omni/Source/OmniForge/Public/OmniForgeModule.h`
#### `Plugins/Omni/Source/OmniForge/Private/OmniForgeModule.cpp`
- Faz:
  - Modulo editor com comando `omni.forge.run`.
  - Parseia argumentos (`root`, `manifestAsset`, `manifestClass`, `requireContentAssets`) e dispara `UOmniForgeRunner::Run`.
- Nao faz:
  - Nao implementa validacao/geracao internamente.

#### `Plugins/Omni/Source/OmniForge/Public/Forge/OmniForgeTypes.h`
#### `Plugins/Omni/Source/OmniForge/Private/Forge/OmniForgeTypes.cpp`
- Faz:
  - DTOs do pipeline (input, normalizado, resolvido, report, erro, resultado).
  - Helpers de settings no struct normalizado.
- Nao faz:
  - Nao roda pipeline.

#### `Plugins/Omni/Source/OmniForge/Public/Forge/OmniForgeRunner.h`
#### `Plugins/Omni/Source/OmniForge/Private/Forge/OmniForgeRunner.cpp`
- Faz:
  - Pipeline completo:
    - Normalize (manifest -> formato canonico)
    - Validate (namespace, systems, IDs, dependencies, ciclo)
    - Resolve (profiles/libraries/assets/rules por system)
    - Generate (`Saved/Omni/ResolvedManifest.json` e `Saved/Omni/Generated/OmniGenerated.json`)
    - Report (`Saved/Omni/ForgeReport.md`)
  - Gera hash deterministico do input normalizado (SHA1).
  - Valida regras de assets/perfis/bibliotecas (Action/Status/Movement).
  - Detecta `ActionId` invalido com prefixo `Input.*`.
  - Produz codigos de erro estruturados (`OMNI_FORGE_E...`).
- Nao faz:
  - Nao altera codigo C++.
  - Nao corrige automaticamente manifest/assets quebrados.

#### `Plugins/Omni/Source/OmniForge/Private/Forge/OmniForgeContext.h`
- Faz:
  - Estado intermediario do pipeline.
- Nao faz:
  - Nao contem regra.

### 4.4 OmniEditor

#### `Plugins/Omni/Source/OmniEditor/OmniEditor.Build.cs`
#### `Plugins/Omni/Source/OmniEditor/Public/OmniEditorModule.h`
#### `Plugins/Omni/Source/OmniEditor/Private/OmniEditorModule.cpp`
- Faz:
  - Modulo editor com casca de startup/shutdown.
- Nao faz:
  - Nao adiciona ferramentas editor concretas nesta versao.

### 4.5 OmniGAS

#### `Plugins/Omni/Source/OmniGAS/OmniGAS.Build.cs`
#### `Plugins/Omni/Source/OmniGAS/Public/OmniGASModule.h`
#### `Plugins/Omni/Source/OmniGAS/Private/OmniGASModule.cpp`
- Faz:
  - Casca de modulo runtime para extensao futura GAS.
- Nao faz:
  - Nao implementa AbilitySystem/GAS flow nesta versao.

### 4.6 Arquivo auxiliar no plugin

#### `Plugins/Omni/Source/OmniRuntime.zip`
- Faz:
  - Snapshot/arquivo compactado de codigo runtime (artefato auxiliar).
- Nao faz:
  - Nao participa do build C++ diretamente.

## 5. Scripts de automacao (`Scripts/`)

### `Scripts/build_incremental.ps1`
- Faz:
  - Alias de compatibilidade para `omni_build_dev.ps1`.
- Nao faz:
  - Nao possui pipeline propria.

### `Scripts/clean_rebuild_normal.ps1`
- Faz:
  - Alias para `omni_build_carimbo.ps1`.
- Nao faz:
  - Nao executa fluxo separado.

### `Scripts/clean_rebuild_hard.ps1`
- Faz:
  - Alias para `omni_build_nuclear.ps1`.
- Nao faz:
  - Nao executa fluxo separado.

### `Scripts/omni_build_dev.ps1`
- Faz:
  - Build incremental editor target.
  - Resolve root, `.uproject`, engine root e chama `Build.bat`.
  - Opcionalmente mata UnrealEditor.
- Nao faz:
  - Nao limpa pastas.
  - Nao regenera project files.

### `Scripts/omni_build_carimbo.ps1`
- Faz:
  - Build "clean de fase".
  - Remove `Intermediate/Binaries` do projeto e do plugin `Omni`.
  - Compila editor target.
- Nao faz:
  - Nao roda zen hygiene.
  - Nao regenera project files.

### `Scripts/omni_build_nuclear.ps1`
- Faz:
  - Pipeline pesada:
    - zen hygiene
    - limpeza de artefatos
    - geracao de project files
    - build
  - Mede tempo total.
- Nao faz:
  - Nao altera regras de projeto; so higiene/build.

### `Scripts/zen_hygiene.ps1`
- Faz:
  - Resolve engine root e executa `zen.exe` (`down`, `service status`) quando disponivel.
  - Mata processos `zenserver` residuais.
  - Opcionalmente mata UnrealEditor.
- Nao faz:
  - Nao recompila codigo.

### `Scripts/omni_conformance_gate.ps1`
- Faz:
  - Gate de conformidade B1/B0.
  - Bloqueia acesso direto indevido a maps protegidos (`Settings/Payload/Arguments/Output`).
  - Bloqueia exposicao `TMap` publico fora de whitelist.
  - Valida `OmniOfficialManifest.cpp` (roots e profile paths esperados).
  - Valida assets esperados em `Content/Data/**` (dependendo de modo CI/flag).
  - Procura `ActionId` `Input.*` em assets default.
  - Valida links Profile -> Library por tokens.
  - Detecta fallback sem guarda de `omni.devdefaults`.
- Nao faz:
  - Nao corrige automaticamente violacoes.

### `Scripts/compare_omni_forge_headless.ps1`
- Faz:
  - Executa forge headless em `UnrealEditor-Cmd`.
  - Pode rodar 1x ou 2x e comparar hash SHA256 de artefato.
  - Salva logs/sumario JSON de comparacao em pasta de diagnostico.
- Nao faz:
  - Nao valida semanticamente o conteudo do JSON alem do hash.

### `Scripts/check_debug_overlay_signals.ps1`
- Faz:
  - Inspeciona log recente e procura sinais esperados do overlay debug.
- Nao faz:
  - Nao verifica estado em memoria; so logs.

### `Scripts/watch_build_errors.ps1`
- Faz:
  - Tail de log em tempo real e destaque de erros/avisos por regex.
- Nao faz:
  - Nao para build nem corrige erro.

### `Scripts/collect_omni_diagnostics.ps1`
- Faz:
  - Coleta logs principais/crash/UBT.
  - Gera `summary.txt` com linhas de erro detectadas.
- Nao faz:
  - Nao diagnostica causa raiz automaticamente.

### `Scripts/git_commit_push.ps1`
- Faz:
  - `git add -A`, commit com mensagem e push (opcionalmente sem push).
  - Resolve branch atual quando nao informado.
- Nao faz:
  - Nao faz review do que esta sendo commitado.
  - Nao restringe por pasta.

### `Scripts/README.md`
- Faz:
  - Documenta modos de build, gate, diagnosticos, determinismo headless e utilitarios.
- Nao faz:
  - Nao executa acao tecnica por si.

## 6. Configuracoes (`Config/`)

### `Config/DefaultGame.ini`
- Faz:
  - Packaging/cultures/settings de projeto.
  - Configura `UOmniSystemRegistrySubsystem` para usar `AutoManifestClassPath=/Script/OmniRuntime.OmniOfficialManifest`.
  - Mantem `bAllowDevDefaults=False` (fail-fast por padrao).
- Nao faz:
  - Nao liga fallback DEV por padrao.

### `Config/DefaultEngine.ini`
- Faz:
  - Define renderer/features e mapas default.
  - Ajusta target settings de plataforma.
- Nao faz:
  - Nao configura regras de systems Omni.

### `Config/DefaultInput.ini`
- Faz:
  - Define axis/input settings do projeto.
- Nao faz:
  - Nao faz binding de gameplay Omni especifico neste arquivo.

### `Config/DefaultGameplayTags.ini`
- Faz:
  - Declara gameplay tags:
    - `State.Exhausted`
    - `Lock.Movement.Sprint`
- Nao faz:
  - Nao impone comportamento (so declaracao de tags).

### `Config/DefaultEditor.ini`
- Faz:
  - Configuracao de localization targets no editor.
- Nao faz:
  - Nao participa da logica runtime.

### `Config/Localization/Game_Gather.ini`
### `Config/Localization/Game_Compile.ini`
### `Config/Localization/Game_GenerateReports.ini`
### `Config/Localization/Game_Export.ini`
### `Config/Localization/Game_Import.ini`
### `Config/Localization/Game_ExportDialogueScript.ini`
### `Config/Localization/Game_ImportDialogue.ini`
### `Config/Localization/Game_ImportDialogueScript.ini`
- Faz:
  - Arquivos gerados pelo Localization Dashboard para gather/import/export/compile de textos.
- Nao faz:
  - Nao contem regra de gameplay.
  - Nao devem ser editados manualmente (comentario do proprio arquivo).

## 7. Dependencia externa (nao autoral)

### `Plugins/Developer/RiderLink/RiderLink.uplugin` + `Plugins/Developer/RiderLink/Source/**`
- Faz:
  - Integracao IPC com JetBrains Rider (debug/log/blueprint/live coding/editor control).
  - Inclui codigo third-party (`RD`, `spdlog`, etc.) e muitos arquivos pregenerated.
- Nao faz:
  - Nao implementa regra de negocio Omni.
  - Nao e o foco de manutencao funcional do projeto.

## 8. Limites atuais do projeto (resumo pragmatico)

- O runtime esta funcional para o loop base `ActionGate <-> Movement <-> Status`, com debug/telemetria e manifesto oficial.
- O que ainda e casca/infra:
  - `OmniEditor` e `OmniGAS` ainda sem logica concreta.
  - `Source/OmniSandbox` e modulo base minimo.
- Forte orientacao fail-fast:
  - Profile/library ausente ou invalido interrompe inicializacao.
  - Schemas invalidos de mensagem sao rejeitados.
- DEV fallback existe, mas explicitamente opt-in e marcado como nao-producao.
