#pragma once

struct NetEngineConfig
{
	NetEngineLoggerConfig logger;
};

class NetEngineInit final
{
public:
	NetEngineInit(const NetEngineInit&) = delete;
	NetEngineInit& operator=(const NetEngineInit&) = delete;
	NetEngineInit(NetEngineInit&&) = delete;
	NetEngineInit& operator=(NetEngineInit&&) = delete;

	explicit NetEngineInit(const NetEngineConfig& config = {});
	~NetEngineInit();
};
