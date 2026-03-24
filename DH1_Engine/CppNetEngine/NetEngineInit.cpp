#include "pch.h"
#include "NetEngineInit.h"

NetEngineInit::NetEngineInit(const NetEngineConfig& config)
{
	SocketUtils::Init();
	NetEngineLogger::Init(config.logger);
}

NetEngineInit::~NetEngineInit()
{
	SocketUtils::Clear();
}
