# TF_REUSE_SOCKET 기반 세션 재사용 설계

> **Status (2026-04):** 설계 완료, 구현 미착수. 매칭 Plan 없음 — 구현 시 Plan 문서 작성 필요.

## Overview

DisconnectEx(TF_REUSE_SOCKET) 완료 후 세션 객체+소켓을 파괴하지 않고, 내부 상태만 리셋하여 재사용 풀에 넣는다. CreateSession() 호출 시 풀에서 먼저 꺼내고, 없으면 새로 생성한다.

## 현재 문제

- DisconnectEx에 TF_REUSE_SOCKET 플래그를 주고 있으나, 재연결 시 CreateSession()이 새 세션+새 소켓을 생성
- 이전 소켓은 버려지고 TIME_WAIT 상태로 ephemeral 포트를 점유
- 5000 동시 연결 + 재연결 반복 시 포트 고갈로 ConnectEx 실패 (ERROR_DUP_NAME 52)

## Session::Reset()

소켓은 유지하고 내부 상태만 초기화하는 새 메서드.

```cpp
bool Session::Reset()
{
    // 상태 전이: None → Disconnected
    if (!setSessionInitialized())
    {
        return false;
    }

    // 새 sessionId 발급
    mSessionId = mpService->AllocateSessionId();
    if (mSessionId == 0)
    {
        return false;
    }

    // 내부 컴포넌트 리셋
    mReceiver.Reset();
    mSender.Reset();
    mTimeoutTracker.Reset();

    return true;
}
```

- CreateSocket() 호출하지 않음 (기존 소켓 유지)
- mpService는 이전 Initialize()에서 이미 설정됨, 유지
- Disconnector, Receiver, Sender의 IocpEvent Owner도 이미 설정됨, 유지
- IOCP 재등록 불필요 (소켓 핸들 유지 → CompletionPort 바인딩 유지)

## Receiver::Reset()

```cpp
void Receiver::Reset()
{
    mNetReceiveBuffer.Clear();
}
```

NetReceiveBuffer에 Clear() 메서드 추가 (읽기/쓰기 포지션 초기화).

## Sender::Reset()

```cpp
void Sender::Reset()
{
    mSendEvent.GetSendPendingBuffer().clear();
    mSendQueue.Clear();
    mbSendRegistered.store(false);
}
```

LockFreeQueue에 Clear() 메서드 추가 (모든 요소 제거).

## SessionTimeoutTracker::Reset()

```cpp
void SessionTimeoutTracker::Reset()
{
    UpdateLastActivityMs();
}
```

## NetService 변경

### 재사용 풀

```cpp
// NetService protected 멤버
LockFreeQueue<SessionRef> mReusableSessionPool;
```

### RecycleSession()

```cpp
void NetService::RecycleSession(const SessionRef& pSession)
{
    pSession->Stop();   // OnDisconnected() + mSessionId=0 + setSessionNone()
    mReusableSessionPool.TryEnqueue(pSession);
}
```

### CreateSession() 수정

```cpp
SessionRef NetService::CreateSession()
{
    SessionRef pSession;

    // 1. 풀에서 재사용 시도
    if (mReusableSessionPool.TryDequeue(pSession))
    {
        if (pSession->Reset())
        {
            return pSession;
        }
        // Reset 실패 시 새로 생성으로 폴백
    }

    // 2. 새로 생성
    pSession = mSessionFactory();
    if (pSession->Initialize(shared_from_this()) == false)
    {
        return nullptr;
    }

    if (mpNetworkScheduler->Register(pSession) == false)
    {
        return nullptr;
    }

    return pSession;
}
```

풀에서 꺼낸 세션은 Initialize()와 IOCP Register()를 건너뛴다 (이미 완료됨).

## 서비스별 OnSessionDisconnected 변경

### ClientService

```cpp
void ClientService::OnSessionDisconnected(const SessionRef& pSession)
{
    mSessionManager.RemoveSession(pSession);
    RecycleSession(pSession);

    mpConnectionManager->FreeConnection();

    if (mbAutoReconnect)
    {
        scheduleReconnect();
    }
}
```

기존: RemoveSession → Stop → (세션 파괴)
변경: RemoveSession → RecycleSession (Stop 호출 후 풀에 넣음)

### ServerService

```cpp
void ServerService::OnSessionDisconnected(const SessionRef& pSession)
{
    mSessionManager.RemoveSession(pSession);
    RecycleSession(pSession);
}
```

동일하게 Stop 대신 RecycleSession.

## 세션 생명주기 (변경 후)

```
[최초 생성]
None → Initialize() → Disconnected → Connected → InGame → Disconnecting → Disconnected
  → Stop() → None → [풀에 넣음]

[재사용]
None → Reset() → Disconnected → Connected → InGame → Disconnecting → Disconnected
  → Stop() → None → [풀에 넣음]

[재사용 반복...]
```

- 최초: Initialize() (소켓 생성 + IOCP 등록 + 컴포넌트 초기화)
- 재사용: Reset() (소켓/IOCP 유지 + 상태/버퍼만 리셋 + 새 sessionId)
- sessionId는 재사용 시마다 새로 발급

## 변경 대상 파일

| 파일 | 변경 내용 |
|------|-----------|
| Session.h/cpp | Reset() 추가 |
| Receiver.h/cpp | Reset() 추가 |
| Sender.h/cpp | Reset() 추가 |
| SessionTimeoutTracker.h/cpp | Reset() 추가 |
| NetReceiveBuffer.h/cpp | Clear() 추가 |
| LockFreeQueue.h | Clear() 추가 |
| NetService.h/cpp | mReusableSessionPool, RecycleSession(), CreateSession() 수정 |
