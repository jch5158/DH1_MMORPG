# Network / Spawn Crash — Residual Risks (Beyond ProtoBridge) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After ruling out ProtoBridge header/`.pb` skew, systematically eliminate or instrument the **remaining** failure modes for `S2C_SPAWN_POSITION_RES` AV and related net-stack instability (relay, framing, UE threading, diagnostics).

**Architecture:** Treat the client as **IOCP receive → sync protobuf parse → handler**. Failures split into **(A) bad bytes on the wire**, **(B) server/gateway bugs**, **(C) UObject/game-thread misuse on net thread**, **(D) unrelated heap corruption**. This plan sequences cheap code checks before heavy tooling (Page Heap).

**Tech Stack:** UE 5.7, CppNetEngine, protobuf 3.21.x (vcpkg), Gateway `S2S_RELAY_TO_CLIENT_NOT`, World movement inner packets.

**Context:** Optional isolated git worktree (@ brainstorming skill) if parallel experiments risk dirtying `main`.

---

## File map (responsibilities)

| File | Why it matters (non–ProtoBridge) |
|------|----------------------------------|
| `DH1_Server/GatewayServer/PacketHandler/GameSessionPacketHandler.cpp` | `payload.size()` → `uint16` relay path; silent truncation / wrong size |
| `DH1_Server/WorldServer/PacketHandler/MovementPacketHandler.cpp` | Inner `MakeMovementSendBuffer` + `RELAY_TO_CLIENT` payload assembly |
| `DH1_Client/.../Network/CppNetEngine/NetSession.cpp` | `header.size` vs `len`; disconnect on handler failure |
| `DH1_Client/.../Network/PacketHandler/MovementPacketHandler.cpp` | Spawn handler; `HANDLE_S2C_MOVE_PATH_RES` and others using `GEngine` |
| `DH1_Client/.../Network/PacketHandler/PacketServiceTypeHandler.h` | Service-type demux; `packetSize` consistency |
| `DH1_Client/ProtoBridge/ProtoBridge.vcxproj` | Optional: add `Echo.pb.cc` to match `ProtocolWrapper.h` (hygiene only) |
| `DH1_Client/Source/DH1_Client/Network/ProtocolHeader/ProtocolWrapper.h` | Includes `Echo.pb.h` — align with ProtoBridge or remove include |

---

### Task 1: Gateway relay — size and failure policy

**Files:**
- Modify: `DH1_Server/GatewayServer/PacketHandler/GameSessionPacketHandler.cpp` (~62–103)

- [ ] **Step 1:** Replace naked `static_cast<uint16>(payload.size())` with `size_t n = payload.size();` and **if `n > 65535`** log `NET_ENGINE_LOG_ERROR` (include gatewaySessionId, size), return `false` (or disconnect policy per project norms).

- [ ] **Step 2:** If `n == 0`, keep current warn + early return.

- [ ] **Step 3:** Build Gateway: `Shared\BuildScripts\BuildAll.bat` (or MSBuild `GatewayServer.vcxproj`).

**Expected:** No silent truncation; large relay attempts surface in logs.

- [ ] **Step 4:** Commit: `fix(gateway): reject oversized relay payloads`

---

### Task 2: End-to-end wire correlation for spawn (client verbose log)

**Files:**
- Modify: `DH1_Client/Source/DH1_Client/Network/PacketHandler/MovementPacketHandler.cpp` (spawn path only) **or** `NetSession.cpp` (guard: movement + spawn id)

- [ ] **Step 1:** On **successful** `ParseFromArray` for `S2C_SPAWN_POSITION_RES` only, log **before** `position()` access: `bodyBytes`, `header id` hex, **first 16–32 bytes** of body as hex (`FString` from `BytesToArray` / small loop).

- [ ] **Step 2:** Compare same session timestamp to World log `LogSpawnPositionResBeforeSend` + serialized size (`ByteSizeLong()` on server before send).

**Run:** Client PIE + local servers, trigger spawn once.

**Expected:** Client `bodyBytes` equals server movement body size; hex prefix matches protobuf tag for field 1 (`0x0A` + len for submessage). Mismatch → fix relay/framing, not client parse.

- [ ] **Step 3:** Commit: `chore(client): verbose spawn-res wire prefix for debugging` (or strip behind `#if !UE_BUILD_SHIPPING` if preferred)

---

### Task 3: UObject / game thread on network thread (movement handlers)

**Files:**
- Modify: `DH1_Client/Source/DH1_Client/Network/PacketHandler/MovementPacketHandler.cpp`

- [ ] **Step 1:** Audit `HANDLE_S2C_MOVE_PATH_RES` (and any sibling) for `GEngine`, `GetWorldContexts`, widget/subsystem calls — list each.

- [ ] **Step 2:** For each, mirror the **spawn** pattern: copy **POD / `std::string` / `TArray` of `FVector`** on net thread, then `AsyncTask(ENamedThreads::GameThread, …)` for UE API only. **Do not** add new public static queue APIs in generated headers.

- [ ] **Step 3:** Run: `Shared\BuildScripts\BuildAll.bat`

**Expected:** No `GEngine`/`UObject` touch from IOCP thread in audited handlers.

- [ ] **Step 4:** Commit: `fix(client): defer movement UE work to game thread`

---

### Task 4: Optional parse hardening (only if Task 2 shows clean wire but crash persists)

**Files:**
- Modify: `DH1_Client/Source/DH1_Client/Network/PacketHandler/MovementPacketHandler.h` (generated — prefer one-off helper in `.cpp` + generator follow-up) **or** template body copy for single packet type

- [ ] **Step 1:** For `S2C_SPAWN_POSITION_RES` only, **copy body** to `std::vector<std::byte>` then `ParseFromArray` from copy (isolates any hypothetical aliasing — low probability if Task 2 clean).

- [ ] **Step 2:** Re-run repro.

**Expected:** If crash unchanged → prioritize Task 5 (heap).

- [ ] **Step 3:** Commit only if kept: `fix(client): copy spawn-res body before protobuf parse`

---

### Task 5: Heap corruption tooling (last resort)

**Files:** None (OS / debugger)

- [ ] **Step 1:** Enable **Full Page Heap** for `UnrealEditor.exe` / `DH1_Client.exe` (Windows GFlags or WER settings per team doc).

- [ ] **Step 2:** Reproduce; if fault moves earlier → fix **first** AV site (buffer overrun in CppNetEngine, memcpy, etc.).

- [ ] **Step 3:** Document finding in `docs/` only if team wants runbooks (YAGNI: skip doc unless requested).

---

### Task 6: ProtoBridge / Echo hygiene (optional, low priority)

**Files:**
- Modify: `DH1_Client/ProtoBridge/ProtoBridge.vcxproj`
- Modify: `DH1_Client/ProtoBridge/ProtoBridge.vcxproj.filters`
- Regenerate or hand-add: `Echo.pb.cc` / `Echo.pb.h` entries mirroring other protos

- [ ] **Step 1:** Add `..\..\Shared\Protocol\Echo.pb.cc` + `.pb.h` to ProtoBridge (same `PrecompiledHeader` = NotUsing pattern as `Enum.pb.cc`).

- [ ] **Step 2:** Run `BuildAll.bat`; confirm no duplicate symbol errors (Echo must not also compile in another static lib linked to client).

**Expected:** `ProtocolWrapper.h` include of `Echo.pb.h` backed by one `.obj` source of truth.

- [ ] **Step 3:** Commit: `chore(protobridge): compile Echo.pb for wrapper consistency`

---

## Plan review loop

- Human or second reviewer: confirm Task 2 evidence before merging Task 4 (avoid speculative copies).
- If Tasks 1–3 fix the symptom, skip Tasks 4–5.

---

## Execution handoff

**Plan complete and saved to** `docs/superpowers/plans/2026-03-31-network-residual-risks-beyond-protobridge.md`.

**Two execution options:**

1. **Subagent-driven (recommended)** — One task per agent; checkpoint after **Task 2** logs. **REQUIRED SUB-SKILL:** superpowers:subagent-driven-development.

2. **Inline execution** — Same session; stop after Task 2 for log review. **REQUIRED SUB-SKILL:** superpowers:executing-plans.

**Which approach?**
