# S2C_SPAWN_POSITION_RES Access Violation — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate `EXCEPTION_ACCESS_VIOLATION` in `MovementPacketHandler::HANDLE_S2C_SPAWN_POSITION_RES` by finding whether the fault is corrupt/ mis-framed wire data, relay truncation, or proto/schema skew, then apply the minimal correct fix (sender, gateway, or parse/validate path).

**Architecture:** The client parses the movement packet body with `google::protobuf` on the IOCP/network dispatch thread (`NetSession::OnReceivePacket` → `PacketServiceTypeHandler` → `MovementPacketHandler::HandlePacket` → `HANDLE_S2C_SPAWN_POSITION_RES`). The crash stack points at the first read of the nested `position` submessage (`packet.position().x()`), i.e. **inside or immediately after** `has_position()` being true and dereferencing the embedded `Vector3`. Buffer lifetime is synchronous for the duration of `OnReceivePacket`, so **post-parse ownership of scalar/submessage data should not alias the receive buffer**; a fault here strongly suggests **invalid protobuf object state after `ParseFromArray` returned true** (wire interpreted under the wrong schema, partial/corrupt payload, or unrelated heap corruption).

**Tech Stack:** Unreal Engine 5.7 (DH1_Client), C++ NetEngine (IOCP), protobuf (ProtoBridge / generated `Movement.pb`), Gateway relay (`S2S_RELAY_TO_CLIENT_NOT`), World `S2C_SPAWN_POSITION_RES` build path.

---

## Crash analysis (current evidence)

| Item | Detail |
|------|--------|
| **Fault** | `EXCEPTION_ACCESS_VIOLATION` reading `0x0000000042958eee` (non-null garbage; typical of corrupted pointer or use-after-free in native heap) |
| **Frame** | `MovementPacketHandler::HANDLE_S2C_SPAWN_POSITION_RES` — source line **66** in `DH1_Client/.../MovementPacketHandler.cpp` |
| **Line 66** | `posX = packet.position().x();` (inside `if (packet.has_position())`) |
| **Thread** | Network scheduler / `PacketSession::OnReceive` — **not** the game thread |
| **Implication** | `ParseFromArray` likely returned **true** (handler entered); nested `position` is **present per API** but internal representation is **invalid** when reading `.x()`, **or** heap was already corrupted earlier |

**Hypotheses (prioritize in this order):**

1. **Wire / schema mismatch** — Bytes on the wire are not a valid encoding of `S2C_SPAWN_POSITION_RES` as the client’s generated `Movement.pb` expects (e.g. field `1` not a valid length-delimited `Vector3` submessage). `ParseFromArray` can still return true while merging unexpected wire into a broken tree depending on version/options.
2. **Relay / framing bug** — `GatewayServer` casts `payload.size()` to `uint16` before send (`GameSessionPacketHandler.cpp` ~88–97). Packets &gt; 65535 bytes truncate silently (unlikely for spawn, but **systemic**). Any other off-by-N strip of header/body would corrupt the body passed to `ParseFromArray`.
3. **Heap corruption elsewhere** — Symptom manifests first here; needs repro with **Full Page Heap** / ASAN-equivalent or binary search logging.
4. **Duplicate / mismatched protobuf runtime** — Rare in single process; verify one libprotobuf / static link story for client.

**Note:** Crash-report account metadata (LoginId / EpicAccountId) is useful only for log correlation; do not commit or paste into public docs.

---

## File map (what touches the bug)

| File | Role |
|------|------|
| `DH1_Client/Source/DH1_Client/Network/PacketHandler/MovementPacketHandler.cpp` | Crash site; spawn handler |
| `DH1_Client/Source/DH1_Client/Network/PacketHandler/MovementPacketHandler.h` | Generated `HandlePacket` template (`ParseFromArray`) |
| `DH1_Client/Source/DH1_Client/Network/PacketHandler/PacketServiceTypeHandler.h` | Service type + `packetSize` vs `len` check |
| `DH1_Client/Source/DH1_Client/Network/CppNetEngine/NetSession.cpp` | Framing; calls `HandlePacketServiceType` |
| `DH1_Engine/CppNetEngine/PacketSession.cpp` | Dispatches `OnReceivePacket` |
| `Shared/Protocol/Proto/Movement.proto` | `S2C_SPAWN_POSITION_RES` schema |
| `Shared/Protocol/Proto/Struct.proto` | `Vector3` definition |
| `DH1_Server/WorldServer/PacketHandler/MovementPacketHandler.cpp` | Builds `S2C_SPAWN_POSITION_RES` + `MakeMovementSendBuffer` |
| `DH1_Server/GatewayServer/PacketHandler/GameSessionPacketHandler.cpp` | `HANDLE_S2S_RELAY_TO_CLIENT_NOT` — memcpy payload to client |
| `Shared/Tools/PacketGenerator/*` | Regenerates client/server handlers if schema or generator changes |

---

### Task 1: Baseline reproduction and binary capture

**Files:** None (runtime only).

- [ ] **Step 1:** Reproduce crash with **Debug** editor/client and note exact build commit and whether PIE or packaged.
- [ ] **Step 2:** Capture **World + Gateway** logs for the same session id / time window (spawn request → relay).
- [ ] **Step 3:** If possible, save a **mini-dump** at crash for post-mortem heap.

**Verify:** Crash still at `MovementPacketHandler.cpp:66` (or shifted line after edits).

- [ ] **Step 4:** Commit (if any logging config / script added): `docs: note repro steps for spawn AV` (optional).

---

### Task 2: Correlation logging (client + server)

**Files:**
- Modify: `DH1_Client/Source/DH1_Client/Network/PacketHandler/MovementPacketHandler.h` (generated — prefer **PacketGenerator** extension or temporary **manual** edit with regen note)
- Modify: `DH1_Client/Source/DH1_Client/Network/PacketHandler/MovementPacketHandler.cpp` — log **before** touching `position()` when debugging

**Approach:** Add a **dev-only** or `LogMovement` **Verbose** log in `HandlePacket` path for **only** `S2C_SPAWN_POSITION_RES`: log `size`, `bodyBytes`, first 24 bytes hex of body, and `header->id` (already available at caller). Alternatively log from `NetSession::OnReceivePacket` for movement packets only (guard by service type parse).

Example (concept — place after `ParseFromArray` succeeds, **before** calling handler):

```cpp
UE_LOG(LogMovement, Verbose,
	TEXT("SPAWN_RES raw: totalSize=%u bodyBytes=%d headId=0x%08X hex=%s"),
	size, bodyBytes, static_cast<uint32>(reinterpret_cast<const PacketHeader*>(pBuffer)->id),
	*BytesToHex(pBuffer + headerBytes, FMath::Min(bodyBytes, 24)));
```

- [ ] **Step 1:** Implement minimal hex + size logging on client for one reproduction.
- [ ] **Step 2:** On World, ensure existing `LogSpawnPositionResBeforeSend` runs for the same flow; compare **serialized body length** (from `ByteSizeLong()` before send) to client `bodyBytes`.
- [ ] **Step 3:** Run: play client + servers, trigger spawn; collect logs.

**Expected:** Client `bodyBytes` matches server serialized size; first bytes should match protobuf tag pattern for field 1 (`0x0A` + length for submessage, etc.). Mismatch → framing/relay bug.

- [ ] **Step 4:** Commit: `chore(net): log spawn position res wire prefix for AV investigation`

---

### Task 3: Audit Gateway relay for truncation and framing

**Files:**
- Modify: `DH1_Server/GatewayServer/PacketHandler/GameSessionPacketHandler.cpp` (lines ~87–98)

- [ ] **Step 1:** Read `payload.size()` as `size_t`; if `payload.size() > std::numeric_limits<uint16>::max()`, log **fatal** and do not send truncated buffer (return false or disconnect policy per project).
- [ ] **Step 2:** Add assert/log when `static_cast<uint16>(payload.size()) != payload.size()` (already covered by max check).
- [ ] **Step 3:** Build server: MSBuild Gateway/World as in `Shared/BuildScripts/BuildAll.bat`.

**Expected:** No silent truncation; if spawn packet ever exceeded 64K, error would be visible.

- [ ] **Step 4:** Commit: `fix(gateway): reject relay payloads larger than uint16 max`

---

### Task 4: Proto / generator alignment audit

**Files:**
- Read: `Shared/Protocol/Proto/Movement.proto`, `Struct.proto`
- Verify: `DH1_Client` and `DH1_Server` both rebuilt after same `PacketGenerator` run (`BuildAll.bat`).

- [ ] **Step 1:** Confirm **single** source of truth: run `Shared/BuildScripts/BuildAll.bat` from clean `Movement.pb` outputs.
- [ ] **Step 2:** Diff **field numbers** for `S2C_SPAWN_POSITION_RES` and `Vector3` between any forked copies (there must be none).
- [ ] **Step 3:** Record `protoc` / libprotobuf versions used by ProtoBridge if documented.

**Expected:** Identical `.pb.h` / `.pb.cc` semantics on both ends.

- [ ] **Step 4:** Commit only if proto or generator changed: `fix(proto): ...`

---

### Task 5: Client-side parse hardening (choose minimal fix based on Task 2)

**Files:**
- Modify: `DH1_Client/Source/DH1_Client/Network/PacketHandler/MovementPacketHandler.h` (template `HandlePacket`) **or** special-case only `S2C_SPAWN_POSITION_RES` in `.cpp` via a small helper to avoid widening all packets.

**Option A — Body copy before parse (isolates aliasing assumptions):**

```cpp
std::vector<std::byte> bodyCopy(bodyBytes);
std::memcpy(bodyCopy.data(), pBuffer + headerBytes, bodyBytes);
PacketType packet{};
if (!packet.ParseFromArray(bodyCopy.data(), bodyBytes)) return false;
return handlePacket(packet, pSession);
```

**Option B — If logs show wrong wire:** fix **sender** (World/Gateway) only; do not add client workarounds beyond `ParseFromArray` failure → disconnect (already partially enforced in `NetSession`).

**Option C — Heap corruption suspected:** run with Windows **Page Heap** on `DH1_Client.exe`; fix any earlier buffer overrun in NetEngine (separate tasks).

- [ ] **Step 1:** If Task 2 proves **good wire**, try Option A and re-run stress test (10+ spawns).
- [ ] **Step 2:** If Task 2 proves **bad wire**, implement **server/gateway fix**; revert Option A if unnecessary (YAGNI).
- [ ] **Step 3:** Run: `Shared/BuildScripts/BuildAll.bat`

**Expected:** No AV; spawn HUD/position still correct.

- [ ] **Step 4:** Commit: `fix(client): harden spawn position parse` or `fix(server): correct spawn relay payload`

---

### Task 6: Threading / UE API audit (secondary)

**Files:**
- Modify: `DH1_Client/.../MovementPacketHandler.cpp` — `HANDLE_S2C_MOVE_PATH_RES` and any handler using `GEngine` on network thread

- [ ] **Step 1:** List handlers that touch `GEngine` / `UObject` from `HandlePacket` (same thread as crash).
- [ ] **Step 2:** Defer to game thread **only** UE-facing calls (pattern already used in spawn handler with `AsyncTask`); do **not** add new public queue APIs — keep deferral **inside** the handler.

**Expected:** No new crash after spawn fix; reduced risk of sporadic UE threading bugs.

- [ ] **Step 3:** Commit separately if changes are non-trivial: `fix(client): defer GEngine work in movement handlers`

---

### Task 7: Regression check

- [ ] **Step 1:** Manual: login → realm → world → **spawn position response** → verify position, name, HP overhead.
- [ ] **Step 2:** Run: `Shared/BuildScripts/BuildAll.bat` — expect success (exit code 0).
- [ ] **Step 3:** Optional: add automated test only if project already has net-level test harness (YAGNI otherwise).

- [ ] **Step 4:** Commit: final integration commit if needed.

---

## Plan review loop

1. After implementation, have a **second engineer** (or human maintainer) read this plan + the actual diff to confirm the chosen fix matches evidence from Task 2 (wire vs heap vs UE thread).
2. If evidence is inconclusive after Task 2–4, **stop** and widen diagnostics (dump + page heap) before merging speculative client copies.

---

## Execution handoff

**Plan complete and saved to** `docs/superpowers/plans/2026-03-31-spawn-position-res-access-violation.md`.

**Two execution options:**

1. **Subagent-driven (recommended)** — Dispatch a fresh subagent per task; review after Tasks 2–3 before coding Task 5. **REQUIRED SUB-SKILL:** superpowers:subagent-driven-development.

2. **Inline execution** — Run tasks in this session with checkpoints after Task 2 logs. **REQUIRED SUB-SKILL:** superpowers:executing-plans.

**Which approach do you want?**
