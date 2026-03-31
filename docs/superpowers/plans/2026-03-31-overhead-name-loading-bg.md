# Overhead name, loading overlay, server verification

> **For agentic workers:** Use @superpowers:subagent-driven-development or @superpowers:executing-plans for any follow-up; steps below are completed unless marked optional.

**Goal:** Show real character names on the overhead HUD, reuse login background on enter-world loading, and make server/DB issues observable.

**Architecture:** Fix empty or stripped display names at the WorldServer DB load path; stop client-side per-TCHAR filtering that could drop valid names; reuse `AuthStyle::GetLoginBgBrush()` for the Slate overlay; log `displayNameLen` on spawn sends.

**Tech stack:** C++ (WorldServer, DH1_Client), MySQL `player_character.character_name`, Slate, existing protobuf `S2C_SPAWN_POSITION_RES.displayName`.

---

### Done in repo

- [x] **WorldServer `GameSessionPacketHandler.cpp`:** `TrimAsciiWhitespaceInPlace` on `character_name` after DB read; if empty after trim, set `Player_<accountId>`; initial spawn log includes `pPlayerObject->GetDisplayName().size()`.
- [x] **WorldServer `MovementPacketHandler.cpp`:** `SPAWN_POSITION_RES` log includes `pPlayer->GetDisplayName().size()`.
- [x] **Client `MovementPacketHandler.cpp`:** `SanitizeOverheadNameUtf8` now UTF-8 → FString + trim only; `NameLooksCorrupted` max length 52; warning if raw UTF-8 non-empty but FString empty.
- [x] **Client `ClientNetSubsystem.cpp`:** Enter-world overlay uses login palette (`AuthStyle::C::ScreenBg`, title text). **Do not** use `GetLoginBgBrush()` here — PNG/transient texture + `UpdateResource` during viewport setup caused in-game entry crashes; `RequestSpawnPosition` defers overlay add to the next game-thread task.

### Optional verification (local)

- [ ] Rebuild `DH1_ClientEditor` and WorldServer on your machine (UE path differs per host).
- [ ] MySQL: `SELECT account_id, character_name, LENGTH(character_name) FROM player_character;` — empty names should no longer matter after server fallback.
- [ ] Watch `logs/WorldServer/NetEngine_*.log` for `displayNameLen` on spawn; client `LogMovement` for `rawNameBytes` / `displayNameOut`.
