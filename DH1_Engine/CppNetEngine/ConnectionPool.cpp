#include "pch.h"
#include "ConnectionPool.h"

ConnectionPool::ConnectionPool(const int32 maxConnectionCount)
	:mMaxConnectionCount(maxConnectionCount)
	, mCurrentConnectionCount(0)
	, mFreeIndexStack(maxConnectionCount)
	, mConnectors()
{
	mConnectors.reserve(maxConnectionCount);
	for (int32 i = 0; i < maxConnectionCount; ++i)
	{
		ConnectorRef pConnector = cpp_net_engine::MakeShared<Connector>(i);
		mConnectors.emplace_back(pConnector);
		(void)mFreeIndexStack.TryPush(i);
	}
}

void ConnectionPool::Dispatch(IocpEvent& iocpEvent, const uint32 numOfBytes)
{
	if (iocpEvent.GetEventType() != eIocpEventType::Connect)
	{
		NET_ENGINE_LOG_FATAL("ConnectionPool::Dispatch - eIocpEventType is not Connect");
		return;
	}

	const auto* pConnectionEvent = static_cast<IocpConnectEvent*>(&iocpEvent);
	mConnectors[pConnectionEvent->GetConnectorIndex()]->Process();
	(void)mFreeIndexStack.TryPush(pConnectionEvent->GetConnectorIndex());
}

ConnectionPoolRef ConnectionPool::GetConnectionManagerRef()
{
	return std::static_pointer_cast<ConnectionPool>(shared_from_this());
}

int32 ConnectionPool::GetMaxConnectionCount() const
{
	return mMaxConnectionCount;
}

bool ConnectionPool::Connect(const ClientServiceRef& pService)
{
	if (mCurrentConnectionCount.fetch_add(1) >= mMaxConnectionCount)
	{
		mCurrentConnectionCount.fetch_sub(1);
		return false;
	}

	int32 index;
	if (mFreeIndexStack.TryPop(index) == false)
	{
		mCurrentConnectionCount.fetch_sub(1);
		return false;
	}

	if (mConnectors[index]->Initialize(GetConnectionManagerRef(), pService) == false)
	{
		(void)mFreeIndexStack.TryPush(index);
		mCurrentConnectionCount.fetch_sub(1);
		return false;
	}

	if (mConnectors[index]->Register() == false)
	{
		(void)mFreeIndexStack.TryPush(index);
		mCurrentConnectionCount.fetch_sub(1);
		return false;
	}

	return true;
}

void ConnectionPool::Close()
{
	mConnectors.clear();
}

void ConnectionPool::FreeConnection()
{
	mCurrentConnectionCount.fetch_sub(1);
}
