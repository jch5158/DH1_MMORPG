#include "pch.h"
#include "Player.h"

Player::Player(const ActorSchedulerRef& pScheduler)
	: Actor(pScheduler)
	, mpSession(nullptr)
{
}

bool Player::SetSession(const PacketSessionRef& pSession)
{
	if (pSession == nullptr || mpSession != nullptr)
	{
		return false;
	}

	mpSession = pSession;
	return true;
}

void Player::ResetSession()
{
	mpSession.reset();
}
