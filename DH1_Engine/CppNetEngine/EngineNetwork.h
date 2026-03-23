#pragma once

#ifndef ENGINE_PCH
#error "EngineNetwork.h must be included after EnginePch.h! Please include 'EnginePch.h' first."
#endif

#include "NetAddress.h"
#include "SocketUtils.h"

#include "IocpCore.h"
#include "IocpEvent.h"
#include "SocketIocpObject.h"
#include "NetworkScheduler.h"

#include "NetReceiveBuffer.h"
#include "NetSendBuffer.h"
#include "Acceptor.h"
#include "Connector.h"
#include "Disconnector.h"
#include "Receiver.h"
#include "Sender.h"
#include "SessionTimeoutTracker.h"
#include "Session.h"
#include "SessionManager.h"
#include "PacketSession.h"

#include "ConnectionPool.h"
#include "Listener.h"
#include "NetService.h"
#include "SessionReaper.h"

#include "ActorService.h"