# Architecture Alignment Pass - Omni (para Mimi)

Data: 2026-03-07  
Base: diagnostico de arquitetura (sem alteracoes de codigo)

## Resumo executivo

O estado atual do Omni esta funcional no recorte principal de runtime: manifest oficial ativo, registry inicializando `5 systems`, bridges operando em smoke tests, e Forge com resultado `pass` em automacao.  

O principal desalinhamento hoje e conceitual/documental:

1. `SYS_Attributes` segue descrito em docs como system dedicado, mas na implementacao atual a responsabilidade de atributos esta consolidada no `Status`.
2. Parte dos documentos de status/mapa de codebase ficou atras do estado real (modulos/systems e checkpoints de roadmap).
3. A regra arquitetural "todo system consome Manifest + Profile + Library" nao esta aplicada de forma uniforme nas areas novas (especialmente `Camera` e `Modifiers`).

Conclusao: a base tecnica esta boa para estabilizacao, mas a camada de governanca/documentacao precisa alinhamento antes de expandir escopo.

## Inventario por area

### 1) Systems

Implementado e funcionando:

- `Status`, `ActionGate`, `Modifiers`, `Movement`, `Camera` registrados no manifest oficial.
- Registry sobe com `systems=5`.
- Logs confirmam init/shutdown e fluxo funcional (sprint/exhausted/camera apply).

Existe so em docs:

- `SYS_Attributes` como system separado (na pratica nao ha classe `UOmniAttributesSystem`).

Parcial:

- `Attributes` funciona, mas como parte do `Status`, nao como modulo/system dedicado.

Desalinhamento:

- Modelo documental de systems nao bate 1:1 com a implementacao atual.

### 2) Bridges

Implementado e funcionando:

- `UOmniAvatarBridgeComponent`
- `UOmniAnimBridgeComponent` + `UOmniAnimInstanceBase`
- `UOmniUIBridgeSubsystem` (HUD)
- `UOmniPlaytestHostSubsystem` (host minimo para PIE)

Parcial:

- Nenhum gap critico no recorte validado; comportamento fallback esta coberto por smoke (`smoke_cmc`, `smoke_fallback`, `smoke_swap`).

Desalinhamento:

- Sem desalinhamento estrutural relevante identificado.

### 3) Libraries

Implementado e funcionando:

- `OmniActionLibrary`, `OmniMovementLibrary`, `OmniStatusLibrary` com assets default em `Content/Data`.

Parcial:

- Nao ha equivalente no mesmo padrao para todas as areas (`Camera` usa rig asset; `Modifiers` sem library dedicada no mesmo formato).

Desalinhamento:

- Em relacao a regra geral de arquitetura, existe heterogeneidade de modelo entre sistemas.

### 4) Profiles

Implementado e funcionando:

- `OmniActionProfile`, `OmniMovementProfile`, `OmniStatusProfile`
- `OmniAttributesRecipeDataAsset` e `OmniCameraRigSpecDataAsset` ativos via config/runtime

Parcial:

- `Camera` e `Modifiers` nao seguem integralmente o mesmo padrao "Profile + Library" dos sistemas canonicos.

Desalinhamento:

- Contrato arquitetural unico nao esta formalizado para as excecoes atuais.

### 5) Manifest

Implementado e funcionando:

- `UOmniOfficialManifest` com entries para `Status`, `ActionGate`, `Modifiers`, `Movement`, `Camera`.
- Ordem/dependencias e bootstrap pelo registry em runtime.

Existe so em docs:

- Referencias antigas que ainda descrevem conjunto menor de systems.

Parcial:

- N/A para funcionalidade principal; manifest esta operacional.

### 6) Forge

Implementado e funcionando:

- Pipeline completo (normalize -> validate -> resolve -> generate -> report).
- Evidencia de automacao em compare mode com hash igual (`pass`).

Parcial:

- Script de conformance ainda valida principalmente o recorte canonico de 3 profiles (`Action`, `Status`, `Movement`).

Desalinhamento:

- Governanca automatizada nao acompanha totalmente o estado atual de runtime (5 systems).

### 7) Experimental

Implementado e funcionando:

- Modulo `OmniExperimental` ativo.
- `UOmniMotionMatchingSubsystem` e comando `omni.mm` funcionando no recorte bootstrap.

Existe so em docs:

- N/A (ha implementacao real e logs de execucao).

Parcial:

- Feature ainda e bootstrap/placeholder, nao fluxo de producao completo.

Desalinhamento:

- Painel geral de status ainda marca P5 como pendente, apesar de evidencias de bootstrap concluido.

## Gaps / inconsistencias (priorizados)

1. **Alta** - `SYS_Attributes` em docs como system dedicado vs implementacao consolidada em `Status`.
2. **Alta** - Documentos centrais desatualizados (mapa/status) em relacao aos modulos/systems atuais.
3. **Media** - Regra arquitetural unica (Manifest + Profile + Library por system) nao cobre explicitamente excecoes reais (`Camera`, `Modifiers`).
4. **Media** - Conformance gate automatizado ainda com cobertura arquitetural parcial (3 profiles canonicos).
5. **Baixa/Media** - Padrao de organizacao documental por pasta de system nao esta uniforme em todas as areas.

## Recomendacao de prioridade (antes de expandir projeto)

### P0 - Alinhamento de modelo canonico

1. Decidir oficialmente: `Attributes` sera system dedicado ou dominio interno de `Status`.
2. Ajustar source-of-truth de arquitetura para refletir essa decisao (docs + roadmap + mapa).

### P0 - Higiene de documentacao de estado

1. Sincronizar `OMNI_PROJECT_STATUS` com evidencias atuais (incluindo P5 bootstrap).
2. Atualizar mapa da codebase para modulos e systems reais.

### P1 - Governanca tecnica

1. Revisar constituicao para explicitar padrao unico vs excecoes aceitas (`Camera`, `Modifiers`).
2. Evoluir conformance gate para validar o estado arquitetural vigente, nao apenas o recorte inicial.

### P1 - Criterios de maturidade

1. Definir checklist objetivo para promover itens de `experimental` para `runtime`.
2. Associar cada promocao a testes de regressao e evidencia de log/automacao.

## Evidencias consultadas

- `Plugins/Omni/Omni.uplugin`
- `Plugins/Omni/Source/OmniRuntime/Private/Manifest/OmniOfficialManifest.cpp`
- `Config/DefaultGame.ini`
- `Saved/Logs/OmniSandbox.log`
- `Saved/Logs/Automation/CharacterAnimPolishP4/*.log`
- `Saved/Logs/Automation/forge_headless/forge_compare_summary.json`
- `Saved/Logs/Automation/MotionMatchingP5/bootstrap.log`
- `Docs/OMNI_PROJECT_STATUS.md`
- `CODEBASE_MAP_EXPLICADO.md`
- `Docs/Governance/OMNI_FRAMEWORK_CONSTITUTION.md`
- `Docs/Governance/OMNI_FOLDER_AND_TAG_STANDARD.md`

