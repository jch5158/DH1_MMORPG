#pragma once

class Player final : public Actor
{
public:

	Player();
	virtual ~Player() override = default;

	bool SetSession(const PacketSessionRef& pSession);
	void ResetSession();

private:

	PacketSessionRef mpSession;
};

