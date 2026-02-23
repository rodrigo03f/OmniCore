# OMNI — ACORDO CANÔNICO DE TRABALHO (A PARTIR DE AGORA)
**Versão:** 1.0  
**Status:** Proposto (pronto para confirmação por: Rodrigo + Codex + Mimi)  
**Escopo:** Regras de governança e engenharia para evitar retrabalho e manter o Omni consistente ao longo dos pulls.

> Objetivo: transformar arquitetura em “lei da física”. Se alguém quebrar, o projeto **não passa** (compile/gate/CI).

---

## 1) Constituição (regras máximas)
1. **Forge gera. Runtime executa. Libraries definem. Profiles ajustam. Manifest manda.**  
2. **Runtime nunca depende de Editor/Forge.**  
3. **Systems não se chamam diretamente.** Comunicação apenas via **Commands / Queries / Events** (+ GameplayTags quando aplicável).  
4. **Fail-fast** é padrão: erro estrutural deve quebrar cedo (init/registry/validator), não “seguir e ver no runtime”.

---

## 2) B0 — Payload/Map Discipline (lei imutável)
### 2.1 Proibido
- Qualquer `TMap` exposto em `public:` em tipos usados por Runtime/Manifest/Message.
- Qualquer API pública que retorne `TMap&`, `const TMap&`, ponteiro ou iterator.
- Qualquer acesso direto a storage interno: `.Add`, `.Find`, `[]`, `.Contains` em `Arguments/Payload/Output/Settings` fora de locais permitidos.

### 2.2 Permitido (único caminho)
- Storage interno sempre `private`.
- Acesso externo apenas via helpers:
  - `SetXxx(Key, Value)`
  - `TryGetXxx(Key, OutValue)`
  - `HasXxx(Key)`
  - `GetKeysSnapshot()` → **snapshot** (TArray), nunca view viva.

### 2.3 Gate obrigatório (anti-regressão)
O repositório deve ter um **gate** (script/CI/pre-commit) que falha se encontrar fora da whitelist:

**Padrões proibidos:**
- `.Settings.Add(` / `.Settings.Find(` / `.Settings[`
- `.Payload.Add(` / `.Payload.Find(` / `.Payload[`
- `.Arguments.Add(` / `.Arguments.Find(` / `.Arguments[`
- `.Output.Add(` / `.Output.Find(` / `.Output[`

**Whitelist (onde pode aparecer):**
- Implementação do próprio envelope/entry encapsulado
- Implementação de schemas (To/TryFrom)
- Testes dedicados (se existirem)

✅ **Definição de “B0 fechado”:** não existe forma de tocar no map direto **e** o gate impede regressão.

---

## 3) Messaging — Command/Query/Event (contrato único)
1. Toda mensagem que atravessa o Runtime tem **Schema tipado**:
   - `Schema::ToMessage(...)`
   - `Schema::TryFromMessage(...)` (ou `FromMessageChecked`)
2. Systems consomem **structs**, não payload cru.
3. Registry/Router valida schema **antes** de rotear (default deny).
4. Nenhum schema trafega `TMap` cru.

---

## 4) B1 — Manifest autoritativo (data-driven) com fallback controlado
1. **Autoritativo = DataAsset `UOmniManifest`.**  
2. `UOmniOfficialManifest` existe apenas como **DEV_FALLBACK** (se usar, logar explicitamente).
3. Systems não carregam regra hardcoded no caminho normal.  
4. Config preferencialmente **tipada**:
   - `TSoftObjectPtr<>` / `TSoftClassPtr<>` ao invés de “string keys”.
5. “Config bag” (K/V) só é permitido se:
   - for encapsulado (B0 compliant)
   - e tiver plano de migração para tipado.

---

## 5) B2 — GameplayTags e namespaces
1. `Omni.*` é **reservado ao framework**.
2. Jogo usa `Game.*` (ou `<Prefix>.*` definido).
3. Tag/ID sem padronização não entra no main: precisa seguir o esquema definido no docs de tags.
4. Qualquer tag “DEV” deve ser marcada como tal ou ficar em namespace apropriado.

---

## 6) “DEV_FALLBACK” (política)
1. Qualquer fallback deve:
   - logar warning contendo `DEV_FALLBACK`
   - ser facilmente desligável por config
2. Release/produção não pode depender do fallback.

---

## 7) Pull Request / Commit (processo para evitar retrabalho)
### 7.1 O que todo PR/commit deve incluir
- **Handoff MD** (curto) com:
  - objetivo do recorte
  - arquivos alterados/novos
  - validação rodada (clean ou incremental)
  - riscos + próximos passos
- Rodar gate de conformidade (B0) antes de pedir review.

### 7.2 Política de merge
- Se quebrar qualquer regra de B0/B1/B2 → **não mergeia**.
- Se for “temporário DEV”: tem que estar explícito com log/flag e tarefa de remoção.

---

## 8) Definições de “Done” por fase
### B0 DONE
- Encapsulamento completo + gate ativo + zero bypass.

### B1 DONE (mínimo)
- Manifest DataAsset autoritativo para os systems alvo.
- Systems sem regra hardcoded (exceto DEV_FALLBACK logado).
- Config tipada ou encapsulada com plano de migração.

### B2 DONE (mínimo)
- Padrão de tags aplicado e validado.
- Namespaces `Omni.*` vs `Game.*` respeitados.

---

## 9) Acordo
Ao confirmar esta versão, qualquer mudança futura:
- entra como “Proposta vX.Y”
- passa por revisão curta (Rodrigo + Codex + Mimi)
- só então vira regra.

**Assinaturas (confirmar):**
- Rodrigo: ☐ Confirmo  
- Codex: ☐ Confirmo  
- Mimi: ☐ Confirmo  
