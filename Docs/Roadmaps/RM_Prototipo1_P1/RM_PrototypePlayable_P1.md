# RM_PrototypePlayable_P1 — Prototype Jogável (Sprint + HP + HUD)

Status: CONCLUÍDO  
Roadmap: RM_PrototypePlayable_P1  
Fase: Transformar MVP técnico em protótipo jogável mínimo  
Data: 02-03-26  

---

## 🎯 Objetivo

Converter o framework atual em um protótipo jogável funcional em poucos dias,
com feedback visual e loop real de gameplay.

---

## 📦 Escopo Congelado

### 1️⃣ SYS_Attributes (Genérico)

DataAsset único:

- DA_AttributesRecipe_Default

Tags oficiais:

- Game.Attr.HP
- Game.Attr.Stamina

Cada atributo possui:

- MaxValue
- StartValue
- Regras opcionais (regen, drain, delay, thresholds)

### Comportamentos

Stamina:
- Drena quando Game.State.Sprinting ativo
- Aplica Game.State.Exhausted quando chega no limite
- Regen após delay
- Remove Exhausted ao recuperar

HP:
- Alterado apenas via comandos (MVP)
- Clamp entre 0 e Max

---

### 2️⃣ SYS_Movement (Walk/Sprint)

Enum interno:
- Walk
- Sprint

Tags externas (sempre 1 ativa):

- Game.Movement.Mode.Walk
- Game.Movement.Mode.Sprint

Tags derivadas:
- Game.State.Sprinting

Speed Pipeline:
EffectiveSpeed = BaseSpeed × ModeMultiplier

BaseSpeed:
- CharacterMovement->MaxWalkSpeed

SprintMultiplier:
- DefaultGame.ini

---

### 3️⃣ SYS_ActionGate

- Decide início/manutenção do Sprint
- Bloqueia quando Game.State.Exhausted ativo
- Retorna decisão determinística com ReasonTag

---

### 4️⃣ HUD Mínima

Exibe:

- Barra HP
- Barra Stamina
- Texto "EXHAUSTED" quando ativo

HUD consome apenas Snapshot do SYS_Attributes.

---

### 5️⃣ Console Commands (Protótipo)

- omni.damage <X>
- omni.heal <X>

Sem aleatoriedade.
Determinístico.

---

## 🔁 Loop Final Esperado

Sprint → Stamina ↓ → Exhausted → Regen → Sprint volta  
Damage → HP ↓ → Heal → HP ↑  

Protótipo deve ser jogável e visualmente validável.

---

## 🧪 Critério de Aceite

- Sprint funcional com bloqueio por stamina
- Regen funcional com delay
- HP responde a comandos
- HUD atualiza corretamente
- Build OK
- Conformance Gate OK
- Smoke OK

---

Roadmap congelado em: 02-03-26

---

## ✅ Fechamento — RM_PrototypePlayable_P1

Data: 2026-03-02  
Status: CONCLUÍDO

### Entregas confirmadas
- [PASS] SYS_Attributes genérico (HP + Stamina) com recipe real carregado via config (sem fallback)
- [PASS] Loop Stamina completo: drain → exhausted → delay → regen → recover
- [PASS] SYS_Movement Walk/Sprint com tags canônicas + speed pipeline
- [PASS] SYS_ActionGate canônico bloqueando sprint por exhausted
- [PASS] Console commands: `omni.damage` e `omni.heal`
- [PASS] HUD mínima (HP + Stamina + EXHAUSTED) consumindo snapshot via Bridge

### Evidências
- Build: `omni_build_dev.ps1` OK
- Conformance: `omni_conformance_gate.ps1` PASS
- Smoke headless: OK (`omni.sprint start/status/stop`, `omni.damage`, `omni.heal`, `quit`)
- PIE: barras de HP/Stamina respondem e EXHAUSTED aparece/some conforme stamina

Conclusão: protótipo jogável mínimo validado end-to-end com feedback visual e loop de gameplay funcional.
