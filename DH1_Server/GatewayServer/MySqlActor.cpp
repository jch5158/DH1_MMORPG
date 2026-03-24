#include "pch.h"
#include "MySqlActor.h"

bool MySqlActor::Initialize(const MySqlConfig& config)
{
	try
	{
		mpConnectionConfig = std::make_shared<sqlpp::mysql::connection_config>();
		mpConnectionConfig->host = config.host;
		mpConnectionConfig->port = config.port;
		mpConnectionConfig->user = config.user;
		mpConnectionConfig->password = config.password;
		mpConnectionConfig->database = config.database;

		mpConnection = std::make_unique<sqlpp::mysql::connection>(mpConnectionConfig);

		NET_ENGINE_LOG_INFO("[MySqlActor] Initialized. host: {}:{}, database: {}", config.host, config.port, config.database);
		return true;
	}
	catch (const sqlpp::exception& e)
	{
		NET_ENGINE_LOG_ERROR("[MySqlActor] Initialization Failed : {}", e.what());
		return false;
	}
}

sqlpp::mysql::connection& MySqlActor::GetConnection()
{
	return *mpConnection;
}
