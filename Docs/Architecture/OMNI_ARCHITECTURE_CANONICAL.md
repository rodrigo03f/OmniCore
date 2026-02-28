# OMNI Architecture Canonical

Date: 2026-02-28
Phase status: B1 HARD CLOSE (2026-02-28)
Public release status: 0.1.0

## Core Contracts

1. Runtime does not depend on Editor or Forge.
2. SystemRegistry keeps deterministic initialization order.
3. Manifest is the source of truth for structural definition.
4. Systems do not call each other directly.
5. Cross-system communication uses Command / Query / Event contracts.
6. No silent fallback in normal runtime path.
7. No implicit system state.

## B1 Architecture Outcomes

- ActionGate is fully asset-driven in normal path.
- Action lifecycle is explicit and observable.
- Movement remains behavior-compatible while consuming lifecycle signals for observability.
- Status is fully data-driven in normal path.
- Forge validation path is hardened with fail-fast behavior.
- Headless determinism verification is externalized via SHA256 compare.
