#pragma once

#include "PacketSession.h"

class SessionValidator
{
public:

	SessionValidator() = delete;
	~SessionValidator() = delete;

	static bool Validate(const PacketSessionRef& pSession)
	{
		if (pSession == nullptr)
		{
			return false;
		}

		if (pSession->IsDisconnectStarted())
		{
			return false;
		}

		return true;
	}
};
