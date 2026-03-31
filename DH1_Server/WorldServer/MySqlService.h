#pragma once

#include "ActorService.h"
#include "ActorLoadBalancer.h"
#include "MySqlActor.h"

class MySqlService final : public std::enable_shared_from_this<MySqlService>
{
public:

	explicit MySqlService(const MySqlConfig& config, ActorServiceRef pActorService);
	~MySqlService() = default;

	[[nodiscard]] bool IsInitialized() const { return mbInitialized; }

	// 라운드 로빈 — 유저 무관한 범용 쿼리용
	template<typename Func>
	void ExecuteAsync(Func&& func)
	{
		if (!mbInitialized)
		{
			return;
		}

		const uint64 actorId = mpLoadBalancer->GetNextRoundRobin();
		PostToActor(actorId, std::forward<Func>(func));
	}

	// 라우팅 키 기반 — 같은 키는 항상 같은 Actor로 전달 (유저별 순서 보장)
	template<typename Func>
	void ExecuteAsync(const uint64 routingKey, Func&& func)
	{
		if (!mbInitialized)
		{
			return;
		}

		const uint64 actorId = mpLoadBalancer->AssignOrGet(routingKey);
		PostToActor(actorId, std::forward<Func>(func));
	}

	// 유저 로그아웃 시 배정 해제 (카운트 감소)
	void ReleaseRoutingKey(const uint64 routingKey);

	// 초기화 단계 등 동기 컨텍스트에서 즉시 단건 쿼리 실행
	template<typename Func>
	bool ExecuteSync(Func&& func)
	{
		if (!mbInitialized || mpLoadBalancer == nullptr)
		{
			return false;
		}

		const uint64 actorId = mpLoadBalancer->GetNextRoundRobin();
		const MySqlActorRef pMySql = GetMySqlActorRef(actorId);
		if (pMySql == nullptr)
		{
			return false;
		}

		func(pMySql->GetConnection());
		return true;
	}

private:
	template<typename Func>
	void PostToActor(const uint64 actorId, Func&& func)
	{
		mpActorService->GetActorDispatcher().Post(actorId, [argService = shared_from_this(), argActorId = actorId, argFunc = std::forward<Func>(func)]()->void
			{
				const MySqlActorRef pMySql = argService->GetMySqlActorRef(argActorId);
				if (pMySql == nullptr)
				{
					return;
				}

				argFunc(pMySql->GetConnection());
			});
	}

	[[nodiscard]] MySqlActorRef GetMySqlActorRef(uint64 actorId);

	bool mbInitialized;
	ActorServiceRef mpActorService;
	std::unique_ptr<ActorLoadBalancer> mpLoadBalancer;
};
