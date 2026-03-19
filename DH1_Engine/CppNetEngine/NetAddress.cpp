#include "pch.h"

#include "NetAddress.h"

NetAddress::NetAddress(const SOCKADDR_IN& sockAddr)
	: mSockAddr(sockAddr)
{
}

NetAddress::NetAddress(const std::string& ip, const uint16 port)
	: mSockAddr{}
{
	mSockAddr.sin_family = AF_INET;
	mSockAddr.sin_addr = IpToAddress(ip);
	mSockAddr.sin_port = ::htons(port);
}

SOCKADDR_IN& NetAddress::GetSockAddr()
{
	return mSockAddr;
}

std::string NetAddress::GetIpAddress() const
{
	char buffer[100];
	InetNtopA(AF_INET, &mSockAddr.sin_addr, buffer, ARRAY_LEN_16(buffer));
	std::string ipAddress(buffer);
	return ipAddress;
}

uint16 NetAddress::GetPort() const
{
	return ::ntohs(mSockAddr.sin_port);
}

void NetAddress::SetSocketAddr(const SOCKADDR_IN& sockAddr)
{
	mSockAddr = sockAddr;
}

IN_ADDR NetAddress::IpToAddress(const std::string& ip)
{
	IN_ADDR address{};
	InetPtonA(AF_INET, ip.c_str(), &address);
	return address;
}
