#pragma once

class JsonConfig
{
public:

	JsonConfig(const JsonConfig&) = default;
	JsonConfig& operator=(const JsonConfig&) = default;
	JsonConfig(JsonConfig&&) = default;
	JsonConfig& operator=(JsonConfig&&) = default;

	static JsonConfig LoadFromFile(const std::string& filePath);

	[[nodiscard]] std::string GetString(const std::string& key) const;
	[[nodiscard]] std::string GetString(const std::string& key, const std::string& defaultValue) const;
	[[nodiscard]] int32 GetInt32(const std::string& key) const;
	[[nodiscard]] int32 GetInt32(const std::string& key, int32 defaultValue) const;
	[[nodiscard]] int64 GetInt64(const std::string& key) const;
	[[nodiscard]] int64 GetInt64(const std::string& key, int64 defaultValue) const;
	[[nodiscard]] uint16 GetUInt16(const std::string& key) const;
	[[nodiscard]] uint16 GetUInt16(const std::string& key, uint16 defaultValue) const;
	[[nodiscard]] uint32 GetUInt32(const std::string& key) const;
	[[nodiscard]] uint32 GetUInt32(const std::string& key, uint32 defaultValue) const;
	[[nodiscard]] bool GetBool(const std::string& key) const;
	[[nodiscard]] bool GetBool(const std::string& key, bool defaultValue) const;
	[[nodiscard]] JsonConfig GetSection(const std::string& key) const;
	[[nodiscard]] bool HasKey(const std::string& key) const;

private:

	explicit JsonConfig(nlohmann::json json);

	static std::string resolveEnvVars(const std::string& value);

	nlohmann::json mJson;
};
