#pragma once

#include "ActorService.h"
#include "MySqlActor.h"

class MySqlService final : public std::enable_shared_from_this<MySqlService>
{
public:

	explicit MySqlService(const MySqlConfig& config, ActorServiceRef pActorService);
	~MySqlService() = default;

	[[nodiscard]] bool IsInitialized() const { return mbInitialized; }
	MySqlActorRef GetMySqlActorRef();

	template<typename Func>
	void ExecuteAsync(Func&& func)
	{
		if (!mbInitialized)
		{
			return;
		}

		mpActorService->GetActorDispatcher().Post(mMySqlActorId, [argService = shared_from_this(), argFunc = std::forward<Func>(func)]()->void
			{
				const MySqlActorRef pMySql = argService->GetMySqlActorRef();
				if (pMySql == nullptr)
				{
					return;
				}

				argFunc(pMySql->GetConnection());
			});
	}

private:
	bool mbInitialized;
	uint64 mMySqlActorId;
	ActorServiceRef mpActorService;
};
