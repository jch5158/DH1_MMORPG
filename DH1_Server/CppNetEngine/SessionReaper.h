#pragma once

class SessionReaper final : public Actor
{
public:

	static constexpr int64 DEFAULT_TIME_OUT = 60000;

	explicit SessionReaper(const ActorContext& actorContext, const int64 timeoutMs);
	virtual ~SessionReaper() override = default;

	[[nodiscard]] int64 GetTimeoutMs() const;
	void ReapSession(const SessionWeak& pSessionWeak) const;

private:

	bool isExpired(const int64 lastActivityMs) const;
	static int64 getNowTimeMs();

	const int64 mTimeoutMs;
};

