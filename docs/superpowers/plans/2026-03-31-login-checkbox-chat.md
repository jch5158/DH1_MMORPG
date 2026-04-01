# Login checkbox ✓ + chat gateway observability — Implementation Plan

> **For agentic workers:** Use subagent-driven-development or executing-plans to implement task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship approved design: email remember checkbox shows a clear ✓ when checked (A), and chat failures matching “local line only + no server logs” are diagnosable via Gateway logs; optionally align client send with `IsInWorld` (B1) or relax validate (B2) after product choice.

**Architecture:** Checked states use a small authored `UTexture2D` brush (box+✓ composite) while unchecked states keep vector rounded-box brushes in `AuthStyle::RememberEmailCheckBoxStyle`. Gateway logs a single structured line when `ChatPacketHandler::Validate` fails before relay.

**Tech Stack:** Unreal Slate (`FCheckBoxStyle`, `FSlateBrush`), DH1 C++ client; DH1 GatewayServer (`ClientSession`, `ChatPacketHandler`).

**Spec:** `docs/superpowers/specs/2026-03-31-login-checkbox-chat-design.md`

---

## File map

| Area | Files |
|------|--------|
| Checkbox style | `DH1_Client/Source/DH1_Client/UI/AuthWidgetStyle.h` |
| Checkbox layout (size slot) | `DH1_Client/Source/DH1_Client/UI/SLoginPanel.cpp` (verify 22×22 still correct) |
| New asset | Import PNG → `UTexture2D` (recommended path: `/Game/UI/Auth/T_LoginEmailCheck` — adjust if project convention differs) |
| Gateway chat | `DH1_Server/GatewayServer/PacketHandler/ChatPacketHandler.cpp` |
| Client guard (B1 only) | `DH1_Client/Source/DH1_Client/Network/Subsystem/ClientNetSubsystem.cpp` (and/or chat UI registration) |

---

### Task 1: Auth checkmark texture asset

**Files:**
- Create: `DH1_Client/Content/UI/Auth/` — import PNG (e.g. 64×64, transparent background, light ✓ + optional border matching theme)
- Reference: same path documented in spec

- [ ] **Step 1:** Create or import a PNG that reads clearly at ~22px (rounded box + ✓ for checked state).
- [ ] **Step 2:** Save Unreal asset; note exact object path (e.g. `/Game/UI/Auth/T_LoginEmailCheck.T_LoginEmailCheck`).
- [ ] **Step 3:** Commit asset + `.uasset` if repo tracks Content.

---

### Task 2: `RememberEmailCheckBoxStyle` — Checked brushes use image

**Files:**
- Modify: `DH1_Client/Source/DH1_Client/UI/AuthWidgetStyle.h`

- [ ] **Step 1:** Add a small helper (e.g. lambda or static local) that builds an `FSlateBrush` with `DrawAs = ESlateBrushDrawType::Image`, `ResourceObject` loaded via `LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Auth/T_LoginEmailCheck.T_LoginEmailCheck"))` (fix path to match Step 1).
- [ ] **Step 2:** Assign `CheckedImage`, `CheckedHoveredImage`, `CheckedPressedImage` to that brush (or tinted variants); set `ImageSize` to `FVector2D(22,22)` or match art.
- [ ] **Step 3:** Keep `Unchecked*` as existing `RoundedBox` fills; tweak border radii if checked bitmap misaligns visually.
- [ ] **Step 4:** Build client editor target; verify login panel checkbox unchecked vs checked in PIE.
- [ ] **Step 5:** Commit: `feat(ui): show checkmark on remember-email checkbox`

---

### Task 3: Gateway — log Validate failure for chat

**Files:**
- Modify: `DH1_Server/GatewayServer/PacketHandler/ChatPacketHandler.cpp`

- [ ] **Step 1:** In `HandlePacket`, when `Validate` returns false **only for the chat service path**, avoid silent drop: either move validation inside `HANDLE_C2S_CHAT_REQ` or add a dedicated pre-check. **Minimal change:** duplicate `Validate` body once in `HandlePacket` before return is messy — cleaner: add `static void LogChatValidateFailure(const PacketSessionRef& pSession)` calling `NET_ENGINE_LOG_WARN` with `accountId` (if logged in), `IsLoggedIn()`, `IsInWorld()`, and `worldServerId` if accessible via `ClientSession` getter (add getter if missing only when necessary).
- [ ] **Step 2:** Call that log from `HandlePacket` when `!Validate(pSession)` for chat map dispatch.
- [ ] **Step 3:** Build GatewayServer via solution that provides `EnginePch.h` include paths.
- [ ] **Step 4:** Reproduce pre-world chat send; confirm WARN line appears.
- [ ] **Step 5:** Commit: `fix(net): log gateway chat validate failures`

---

### Task 4 (optional branch B1): Client — do not send chat until in-world

**Prerequisite:** Product confirms B1 (no lobby chat).

**Files:**
- Modify: `DH1_Client/Source/DH1_Client/Network/Subsystem/ClientNetSubsystem.cpp` (`SendChatRequest` or callers)

- [ ] **Step 1:** Identify authoritative client flag for “in world” (e.g. post spawn / session state already used for movement). If none, add a bool set when game session enter completes (match Gateway `mWorldServerId` semantics as closely as possible).
- [ ] **Step 2:** If not in world, return false from `SendChatRequest` and skip optimistic `AppendChatLine` in `TrySendChatFromInput` (or show a short local message — product choice).
- [ ] **Step 3:** PIE: before world, send blocked; after world, send works and server logs appear.
- [ ] **Step 4:** Commit: `fix(client): gate chat send until in world`

---

### Task 4 (optional branch B2): Gateway — relax Validate

**Prerequisite:** Product confirms B2.

**Files:**
- Modify: `DH1_Server/GatewayServer/PacketHandler/ChatPacketHandler.cpp` (`Validate`)

- [ ] **Step 1:** Change `Validate` to match policy (e.g. `IsLoggedIn()` only).
- [ ] **Step 2:** Define behavior when `accountId` not in world relay (World may reject — verify `GameSessionPacketHandler` relay path).
- [ ] **Step 3:** Test and commit with message explaining policy.

---

## Verification

- **UI:** Login → toggle remember email → ✓ visible when checked.
- **Chat:** Two clients in same AOI; send message; both see line on recipients; Gateway log contains relay line; if validate failed before fix, WARN explains `!IsInWorld`.

---

## Commits

Prefer two commits minimum: (1) Gateway logging, (2) Client checkbox. Optional third: B1 or B2 branch.
