#include "pch.h"
#include "JsonConfig.h"

#include <fstream>
#include <regex>

JsonConfig JsonConfig::LoadFromFile(const std::string& filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open())
	{
		NET_ENGINE_LOG_FATAL("JsonConfig::LoadFromFile - Failed to open file: {}", filePath);
		throw std::runtime_error("Failed to open config file: " + filePath);
	}

	nlohmann::json json = nlohmann::json::parse(file);
	return JsonConfig(std::move(json));
}

std::string JsonConfig::GetString(const std::string& key) const
{
	return resolveEnvVars(mJson.at(key).get<std::string>());
}

std::string JsonConfig::GetString(const std::string& key, const std::string& defaultValue) const
{
	if (!mJson.contains(key))
	{
		return defaultValue;
	}

	return resolveEnvVars(mJson[key].get<std::string>());
}

int32 JsonConfig::GetInt32(const std::string& key) const
{
	return mJson.at(key).get<int32>();
}

int32 JsonConfig::GetInt32(const std::string& key, const int32 defaultValue) const
{
	if (!mJson.contains(key))
	{
		return defaultValue;
	}

	return mJson[key].get<int32>();
}

int64 JsonConfig::GetInt64(const std::string& key) const
{
	return mJson.at(key).get<int64>();
}

int64 JsonConfig::GetInt64(const std::string& key, const int64 defaultValue) const
{
	if (!mJson.contains(key))
	{
		return defaultValue;
	}

	return mJson[key].get<int64>();
}

uint16 JsonConfig::GetUInt16(const std::string& key) const
{
	return mJson.at(key).get<uint16>();
}

uint16 JsonConfig::GetUInt16(const std::string& key, const uint16 defaultValue) const
{
	if (!mJson.contains(key))
	{
		return defaultValue;
	}

	return mJson[key].get<uint16>();
}

uint32 JsonConfig::GetUInt32(const std::string& key) const
{
	return mJson.at(key).get<uint32>();
}

uint32 JsonConfig::GetUInt32(const std::string& key, const uint32 defaultValue) const
{
	if (!mJson.contains(key))
	{
		return defaultValue;
	}

	return mJson[key].get<uint32>();
}

bool JsonConfig::GetBool(const std::string& key) const
{
	return mJson.at(key).get<bool>();
}

bool JsonConfig::GetBool(const std::string& key, const bool defaultValue) const
{
	if (!mJson.contains(key))
	{
		return defaultValue;
	}

	return mJson[key].get<bool>();
}

JsonConfig JsonConfig::GetSection(const std::string& key) const
{
	return JsonConfig(mJson.at(key));
}

bool JsonConfig::HasKey(const std::string& key) const
{
	return mJson.contains(key);
}

JsonConfig::JsonConfig(nlohmann::json json)
	: mJson(std::move(json))
{
}

std::string JsonConfig::resolveEnvVars(const std::string& value)
{
	static const std::regex envPattern(R"(\$\{(\w+)\})");
	std::string result = value;
	std::smatch match;

	while (std::regex_search(result, match, envPattern))
	{
		const std::string envName = match[1].str();
		char* envValue = nullptr;
		size_t envLen = 0;
		const errno_t err = _dupenv_s(&envValue, &envLen, envName.c_str());

		if (err == 0 && envValue != nullptr)
		{
			result = match.prefix().str() + envValue + match.suffix().str();
			free(envValue);
		}
		else
		{
			NET_ENGINE_LOG_ERROR("JsonConfig::resolveEnvVars - Environment variable '{}' not found", envName);
			result = match.prefix().str() + match.suffix().str();
		}
	}

	return result;
}
