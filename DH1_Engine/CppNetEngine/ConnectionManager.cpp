#include "pch.h"
#include "ConnectionManager.h"

ConnectionManager::ConnectionManager(const int32 connectCount)
	:mConnectCount(connectCount)
	, mFreeIndexStack(connectCount)
	, mConnectors()
{
	mConnectors.reserve(connectCount);
}

void ConnectionManager::Dispatch(IocpEvent& iocpEvent, const uint32 numOfBytes)
{
	if (iocpEvent.GetEventType() != eIocpEventType::Connect)
	{
		NET_ENGINE_LOG_FATAL("ConnectionManager::Dispatch - eIocpEventType is not Connect");
		return;
	}

	const auto* pConnectionEvent = static_cast<IocpConnectEvent*>(&iocpEvent);
	mConnectors[pConnectionEvent->GetConnectorIndex()]->Process();
}

ConnectionManagerRef ConnectionManager::GetConnectionManagerRef()
{
	return std::static_pointer_cast<ConnectionManager>(shared_from_this());
}

bool ConnectionManager::Connect(const ClientServiceRef& pService)
{
	for (int32 i = 0; i < mConnectCount; ++i)
	{
		ConnectorRef pConnector = cpp_net_engine::MakeShared<Connector>(i);
		if (pConnector->Initialize(GetConnectionManagerRef(), pService) == false)
		{
			NET_ENGINE_LOG_ERROR("ConnectionManager::Connect - pConnector->Initialize is failed");
			continue;
		}

		mConnectors.emplace_back(pConnector);
		pConnector->Register();
	}

	return true;
}

void ConnectionManager::Connect(const ClientServiceRef& pService, const int32 connectCount)
{
	const int32 freeConnectorCount = mFreeIndexStack.Count();
	const int32 remainConnectCount = std::min(freeConnectorCount, connectCount);

	for (int32 i = 0; i < remainConnectCount; ++i)
	{
		int32 index;
		if (mFreeIndexStack.TryPop(index) == false)
		{
			break;
		}

		mConnectors[index]->Register();
	}

	return;
}

void ConnectionManager::Close()
{
	mConnectors.clear();
}
