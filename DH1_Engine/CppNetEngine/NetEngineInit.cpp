#include "pch.h"
#include "NetEngineInit.h"

NetEngineInit::NetEngineInit(const bool isUnrealClient)
{
	SocketUtils::Init();
	NetEngineLogger::Init(isUnrealClient);
}

NetEngineInit::~NetEngineInit()
{
	SocketUtils::Clear();
}
