#pragma once

class NetAddress
{
public:
	
	NetAddress(NetAddress&&) = delete;
	NetAddress& operator=(NetAddress&&) = delete;

	explicit NetAddress() = default;
	explicit NetAddress(const SOCKADDR_IN& sockAddr);
	explicit NetAddress(const std::string& ip, const uint16 port);
	explicit NetAddress(const NetAddress& netAddress) = default;
	NetAddress& operator=(const NetAddress&) = default;

	[[nodiscard]] SOCKADDR_IN& GetSockAddr();
	[[nodiscard]] std::string GetIpAddress() const;
	[[nodiscard]] uint16 GetPort() const;

	void SetSocketAddr(const SOCKADDR_IN& sockAddr);

	static IN_ADDR IpToAddress(const std::string& ip);

private:

	SOCKADDR_IN	mSockAddr;
};

