# RM_PrototypePlayable_P1 — Prototype Jogável (Sprint + HP + HUD)

Status: DESIGN FROZEN  
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
