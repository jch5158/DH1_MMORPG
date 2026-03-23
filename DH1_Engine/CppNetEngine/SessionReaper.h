#pragma once

class SessionReaper final
{
public:

	static constexpr int64 DEFAULT_TIMEOUT_MS = 60000;
	static constexpr int64 ABORT_TIMEOUT_MS = 10000;
	static constexpr int64 SWEEP_INTERVAL_MS = 5000;

	explicit SessionReaper(const int64 timeoutMs = DEFAULT_TIMEOUT_MS);
	~SessionReaper() = default;

	void Sweep(const ServerServiceWeak& pServerServiceWeak);
	void SweepAbort();

	[[nodiscard]] int64 GetTimeoutMs() const;
	[[nodiscard]] static constexpr int64 GetSweepIntervalMs() { return SWEEP_INTERVAL_MS; }

private:

	struct AbortEntry
	{
		SessionWeak pSessionWeak;
		int64 disconnectStartMs;
	};

	[[nodiscard]] bool isExpired(const int64 lastActivityMs) const;
	[[nodiscard]] static bool isAbortExpired(const int64 disconnectStartMs);
	[[nodiscard]] static int64 getNowTimeMs();

	const int64 mTimeoutMs;

	SharedMutex mAbortLock;
	Vector<AbortEntry> mAbortPendingSessions;
};
