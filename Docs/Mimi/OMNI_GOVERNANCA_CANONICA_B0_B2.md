# OMNI - Governanca Canonica (B0 -> B2)

## 1) Objetivo
Definir regras tecnicas obrigatorias para evitar desconformidade entre implementacao e arquitetura.

Este documento deve ser tratado como fonte canonica entre Codex e Mimi.

## 2) Regras obrigatorias (nao negociaveis)

### R1) Encapsulamento de payload/config map
- Proibido expor `TMap` em `public:` nos contratos de runtime.
- Proibido retornar `TMap&`, `const TMap&`, ponteiro ou iterator em API publica.
- Permitido apenas acesso por helpers:
  - `SetXxx(...)`
  - `TryGetXxx(...)`
  - `HasXxx(...)`
  - `GetKeysSnapshot()`

### R2) Messaging 100% tipada
- Toda mensagem entre systems deve passar por schema tipado (`ToMessage`/`TryFromMessage`).
- System nao pode acessar payload cru diretamente.
- Registry valida schema antes de rotear (fail-fast).

### R3) Manifest/Profile/Library como fonte autoritativa
- Dados de comportamento devem vir de Manifest/Profile/Library.
- Hardcode de regra de gameplay em system: proibido em caminho normal.
- Hardcode permitido apenas em `DEV_FALLBACK` explicitamente logado.

### R4) DEV_FALLBACK explicito
- Todo fallback deve gerar log `Warning` com texto `DEV_FALLBACK`.
- Fallback deve ser facil de desativar em config.
- Build de release nao pode depender de fallback.

### R5) Contrato de configuracao tipado
- Evitar "config bag" stringly-typed.
- Quando possivel, usar campos tipados (`TSoftObjectPtr`/`TSoftClassPtr`) no manifest/profile.
- Se chave/valor for necessario no curto prazo, encapsular e declarar plano de migracao.

### R6) Naming e namespaces
- `Omni.*` reservado para framework.
- Conteudo de jogo deve usar namespace separado (ex.: `Game.*`).
- IDs e tags devem ter convencao documentada e validada.

### R7) Politica de build
- Fluxo diario: build incremental.
- Clean rebuild: somente em excecao (UHT/linker inconsistente, troca de engine, erro persistente).

## 3) Gate de conformidade (obrigatorio)

Adicionar script/checagem com falha automatica para padroes proibidos fora dos arquivos permitidos.

### 3.1) Padroes proibidos
- `.Settings.Add(` / `.Settings.Find(` / `.Settings[`
- `.Payload.Add(` / `.Payload.Find(` / `.Payload[`
- `.Arguments.Add(` / `.Arguments.Find(` / `.Arguments[`
- `.Output.Add(` / `.Output.Find(` / `.Output[`

### 3.2) Locais permitidos (whitelist)
- Implementacao do envelope/helper.
- Implementacao de schema.
- Testes de conformidade (se existirem).

## 4) Processo de mudanca (Codex + Mimi)

### 4.1) Antes de codar
- Registrar mini-RFC no ticket/MD com:
  - objetivo
  - impacto em contrato
  - risco de regressao
  - criterio de aceite

### 4.2) Durante a mudanca
- Se tocar em contrato publico, atualizar este documento no mesmo PR/commit.
- Se adicionar fallback, log `DEV_FALLBACK` e criterio de remocao.

### 4.3) Antes de merge
- Passar checklist de release tecnico:
  - build OK
  - gate de conformidade OK
  - sem bypass direto de map
  - docs atualizadas

## 5) Definicao de pronto por fase

### B0 (disciplina de payload/map)
- Zero `TMap` cru exposto em API publica de runtime/envelope/manifest entry.
- Gate automatizado ativo.
- Bypass direto nao compila ou falha no gate.

### B1 (data-driven)
- Action/Status/Movement configurados por Manifest/Profile/Library.
- Fallback apenas DEV, com log explicito.
- Caminho autoritativo via assets/classe tipada.

### B2 (hardening)
- Padronizacao final de tags e IDs.
- Validacao de naming/versionamento.
- Reducao progressiva de stringly-typed configs.

## 6) Roadmap canonico (alto nivel)

1. Fechar B0 de forma estrita (encapsulamento + gate).
2. Consolidar B1 recorte 02 (assets reais e contratos tipados).
3. Expandir B1 para todos os systems core.
4. Endurecer B2 (naming/tags/versionamento + testes de regressao).

## 7) Itens para alinhamento com Mimi (responder SIM/NAO)
- `Settings` fica encapsulado imediatamente no proximo recorte?
- Gate de conformidade entra como obrigatorio antes de novo merge de runtime?
- `DEV_FALLBACK` fica permitido apenas para `Development`/`Editor`?
- Contrato tipado substitui chaves stringly-typed em B1 recorte 02?
- Este arquivo vira referencia oficial para decisao de arquitetura?
