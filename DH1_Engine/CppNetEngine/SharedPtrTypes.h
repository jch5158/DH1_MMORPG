#pragma once

#define DECLARE_SHARED_PTR(TypeName) \
	class TypeName; /*NOLINT(bugprone-macro-parentheses)*/ \
	using TypeName##Ref = SharedPtr<TypeName>; \
	using TypeName##ConstRef = SharedPtr<const TypeName>;

#define DECLARE_SMART_PTR(TypeName) \
	DECLARE_SHARED_PTR(TypeName) \
	using TypeName##Weak = WeakPtr<TypeName>; \
	using TypeName##ConstWeak = WeakPtr<const TypeName>;

DECLARE_SMART_PTR(IocpObject);
DECLARE_SMART_PTR(SocketIocpObject)
DECLARE_SMART_PTR(Listener);
DECLARE_SMART_PTR(ConnectionPool);
DECLARE_SMART_PTR(Acceptor);
DECLARE_SMART_PTR(Connector);
DECLARE_SMART_PTR(Session);
DECLARE_SMART_PTR(GameSession);
DECLARE_SMART_PTR(PacketSession);
DECLARE_SMART_PTR(NetSendBuffer);

DECLARE_SMART_PTR(IActor);
DECLARE_SMART_PTR(Actor);
DECLARE_SMART_PTR(ScopedActor);
DECLARE_SMART_PTR(Message);

DECLARE_SMART_PTR(SessionReaper);
DECLARE_SMART_PTR(SessionManager);
DECLARE_SMART_PTR(WaitQueueManager);

DECLARE_SMART_PTR(IocpCore);
DECLARE_SMART_PTR(NetworkScheduler);
DECLARE_SMART_PTR(ActorScheduler);

DECLARE_SMART_PTR(NetService);
DECLARE_SMART_PTR(ClientService);
DECLARE_SMART_PTR(ServerService);
