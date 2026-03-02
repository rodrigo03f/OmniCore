# RM_InteligenciaSistemica — Execution Order Oficial

Status: CONGELADO  
Data: 02-03-26  

---

## 🎯 Objetivo

Definir a ordem oficial de execução dos sistemas da RM_InteligenciaSistemica,
evitando dependência circular, profundidade prematura e cruzamentos caóticos.

Regra central:
Executar U1 completo de cada sistema antes de aprofundar qualquer um.

---

# 🧭 Ordem Oficial de Execução

## 1️⃣ SYS_ActionGate — U1

Responsabilidade:
Sistema de decisão e controle de ações.

Critério para avançar:

- ReasonTag sempre preenchido
- Locks globais e por ação funcionando
- Determinismo validado
- Conformance OK
- Smoke OK

Somente após isso seguir para o próximo sistema.

---

## 2️⃣ SYS_Attributes — U1

Responsabilidade:
Sistema genérico de atributos (HP + Stamina).

Critério para avançar:

- DA_AttributesRecipe_Default funcionando
- Stamina drain / delay / regen operando
- Game.State.Exhausted publicado corretamente
- Snapshot estável
- Build OK
- Smoke OK

---

## 3️⃣ SYS_Movement — U1

Responsabilidade:
Execução física de Walk/Sprint com pipeline de velocidade.

Critério para avançar:

- Mode sempre definido (Walk ou Sprint)
- Game.State.Sprinting publicado corretamente
- Speed = BaseSpeed × Multiplier funcionando
- Respeita Gate + Exhausted
- Build OK
- Smoke OK

---

## 4️⃣ SYS_Camera — U1

Responsabilidade:
Aplicação de CameraRigSpec via Mode tags.

Critério para avançar:

- Mode tags selecionam rig corretamente
- Fallback seguro funcionando
- Snapshot consistente
- Build OK
- Smoke OK

---

# 🧠 Regra Arquitetural

Ordem conceitual:

Decidir → Regular → Executar → Apresentar

Traduzido:

ActionGate → Attributes → Movement → Camera

Nenhum sistema deve avançar para U2
antes que todos tenham U1 fechado.

---

# 🚀 Próximo Marco

Início oficial da execução desta ordem:
03-03-26

Roadmap preparado para execução disciplinada e progressiva.
