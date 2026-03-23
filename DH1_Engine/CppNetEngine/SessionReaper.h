#pragma once

class SessionReaper final
{
public:

	static constexpr int64 DEFAULT_TIME_OUT_MS = 60000;
	static constexpr int64 ABORT_TIME_OUT_MS = 10000;

	explicit SessionReaper(const int64 timeoutMs = DEFAULT_TIME_OUT_MS);
	~SessionReaper() = default;

	[[nodiscard]] int64 GetTimeoutMs() const;
	[[nodiscard]] static constexpr int64 GetAbortTimeoutMs() { return ABORT_TIME_OUT_MS; }
	void ReapSession(const ServerServiceWeak& pServerServiceWeak, const SessionWeak& pSessionWeak) const;
	static void AbortSession(const SessionWeak& pSessionWeak);

private:

	[[nodiscard]] bool isExpired(const int64 lastActivityMs) const;
	static int64 getNowTimeMs();

	const int64 mTimeoutMs;
};

