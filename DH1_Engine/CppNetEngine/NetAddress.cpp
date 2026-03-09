#include "pch.h"

#include "NetAddress.h"

NetAddress::NetAddress(const SOCKADDR_IN& sockAddr)
	: mSockAddr(sockAddr)
{
}

NetAddress::NetAddress(const Wstring& ip, const uint16 port)
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

Wstring NetAddress::GetIpAddress() const
{
	WCHAR buffer[100];
	InetNtopW(AF_INET, &mSockAddr.sin_addr, buffer, ARRAY_LEN_16(buffer));

	return { buffer };
}

uint16 NetAddress::GetPort() const
{
	return ::ntohs(mSockAddr.sin_port);
}

void NetAddress::SetSocketAddr(const SOCKADDR_IN& sockAddr)
{
	mSockAddr = sockAddr;
}

IN_ADDR NetAddress::IpToAddress(const Wstring& ip)
{
	IN_ADDR address{};
	InetPtonW(AF_INET, ip.c_str(), &address);

	return address;
}
