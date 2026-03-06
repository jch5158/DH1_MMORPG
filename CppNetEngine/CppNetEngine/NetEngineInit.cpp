#include "pch.h"
#include "NetEngineInit.h"
#include "SocketUtils.h"

NetEngineInit::NetEngineInit()
{
	SocketUtils::Init();
	NetEngineLogger::Init();
}

NetEngineInit::~NetEngineInit()
{
	SocketUtils::Clear();
}
