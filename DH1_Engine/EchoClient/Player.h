#pragma once
#include "Actor.h"

class Player final : public Actor
{
public:

	explicit Player();
	virtual ~Player() override = default;

	bool SetSession(const PacketSessionRef& pSession);
	void ResetSession();

private:

	PacketSessionRef mpSession;
};

