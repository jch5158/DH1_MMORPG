#pragma once

class RelayContext
{
public:

	static void SetAccountId(const uint64 accountId) { sAccountId = accountId; }
	[[nodiscard]] static uint64 GetAccountId() { return sAccountId; }
	static void Clear() { sAccountId = 0; }

private:

	inline static thread_local uint64 sAccountId = 0;
};
