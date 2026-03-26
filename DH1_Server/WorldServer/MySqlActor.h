#pragma once
#include <Actor.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>

struct MySqlConfig
{
	std::string host;
	uint16 port = 3306;
	std::string user;
	std::string password;
	std::string database;
};

class MySqlActor final : public Actor
{
public:
	MySqlActor() = default;
	virtual ~MySqlActor() override = default;

	MySqlActor(const MySqlActor&) = delete;
	MySqlActor& operator=(const MySqlActor&) = delete;
	MySqlActor(MySqlActor&&) = delete;
	MySqlActor& operator=(MySqlActor&&) = delete;

	bool Initialize(const MySqlConfig& config);

	[[nodiscard]] sqlpp::mysql::connection& GetConnection();

private:
	std::shared_ptr<sqlpp::mysql::connection_config> mpConnectionConfig;
	std::unique_ptr<sqlpp::mysql::connection> mpConnection;
};
