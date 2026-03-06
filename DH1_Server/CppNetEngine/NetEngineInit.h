#pragma once

class NetEngineInit final
{
public:
	NetEngineInit(const NetEngineInit&) = delete;
	NetEngineInit& operator=(const NetEngineInit&) = delete;
	NetEngineInit(NetEngineInit&&) = delete;
	NetEngineInit& operator=(NetEngineInit&&) = delete;

	NetEngineInit();
	~NetEngineInit();
};