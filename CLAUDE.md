# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

**Engine solution** (Visual Studio, MSBuild):
```
DH1_Engine/DH1_Engine.slnx          # CppNetEngine (static lib), EchoServer, EchoClient
DH1_Server/GatewayServer/           # Separate project, links CppNetEngine
```

**vcpkg manifest**: `Shared/vcpkg/vcpkg.json` — dependencies: crashpad, fmt, nlohmann-json, mimalloc, spdlog, protobuf, utfcpp, redis-plus-plus.

**Packet code generation** (must run after `.proto` changes):
```batch
cd Shared/BuildScripts
PacketGenerator.bat    # protoc → PacketGenerator.exe → xcopy to DH1_Client
```
PacketGenerator is a C# .NET 10 tool in `Shared/Tools/PacketGenerator/`.

## Architecture

### Engine (DH1_Engine/CppNetEngine)

IOCP-based async networking engine providing:
- **NetService hierarchy**: `NetService` → `ServerService` (listener + session reaper) / `ClientService` (connection pool + auto-reconnect with backoff)
- **Actor system**: `Actor` with `ActorMailbox` (lock-free queue), `ActorDispatcher` (type-safe Post/PostDelay), `ActorScheduler` (IOCP + TimingWheel)
- **Packet pipeline**: 5-byte header `[size:uint16][id:uint32]`, `PacketSession` base class auto-parses from receive buffer
- **Memory**: `ObjectPool` (lock-free, Node16 ABA protection, checksum guards), `TlsObjectPool` (per-thread chunks), `MemoryAllocator` (tiered: 256B stride → 4KB stride → mimalloc fallback)
- **Lock-free structures**: `LockFreeStack`, `LockFreeQueue` using 128-bit CAS with counter

### Service Lifecycle Pattern

All services follow **2-phase initialization**: `Initialize(config)` → `Start()` → `Run()` → `Stop()`. Server processes (GatewayServer, EchoServer) create a `*Service` class that owns all dependencies and manages lifecycle.

```
GatewayService owns: ServerService, ActorService, RedisService
EchoServerService owns: ServerService
```

### Packet System

Packets defined in `Shared/Protocol/Proto/*.proto` with custom options (`service_type`, `handler_name`, `sender`, `receiver`). Packet IDs are **auto-incremented** per handler by message order in the proto file — do not add `packet_id` options manually.

Generated output: `PacketId.h` (enums), `*Handler.h` (dispatch), `PacketServiceTypeHandler.h` (service type → handler routing). Generated code includes `AUTO-GENERATED DO NOT EDIT` warnings.

### Configuration

JSON configs in `Shared/Config/Server/` and `Shared/Config/Client/`, loaded via `JsonConfig::LoadFromFile()`. Supports dot-notation access (`config.GetInt32("server.port")`) and section extraction (`config.GetSection("networkScheduler")`).

## Conventions

- **Line endings**: CRLF (`\r\n`) for all generated and source files. `PacketFormatter.NormalizeToCRLF()` enforces this in generated code.
- **Memory fences**: Use `std::memory_order_acquire` for LoadLoad ordering (count-before-pointer pattern in lock-free structures). Do not use `seq_cst` unless StoreLoad ordering is needed.
- **Aggregate headers**: `EnginePch.h` (core), `EngineNetwork.h` (network), `EngineActor.h` (actor). Each requires `EnginePch.h` first (compile-time guard via `#ifndef ENGINE_PCH`).
- **Naming**: `mp` prefix for member pointers, `mb` for member bools, `m` for other members. `Ref` suffix for `shared_ptr` types, `Weak` for `weak_ptr`.
- **Smart pointers**: Use `cpp_net_engine::MakeShared<T>()` (ObjectPool-backed). Raw `new`/`delete` should not appear in engine code.
- **Iterator naming**: Use `iter` (not `it`) for iterator variables.
- **Service type enums**: Must match proto `eServiceType` values exactly.
