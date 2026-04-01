#include "pch.h"
#include "WorldService.h"
#include "GatewaySession.h"
#include "RealmSession.h"
#include "RedisService.h"
#include "MySqlService.h"
#include "GameSessionManager.h"
#include "GridManager.h"
#include "NavMeshManager.h"
#include "GameTickProcessor.h"
#include "PacketHandler/GameSessionPacketHandler.h"
#include "GameObject.h"
#include "NetService.h"
#include "NetworkScheduler.h"
#include "ActorService.h"
#include "ThreadManager.h"
#include "JsonConfig.h"
#include "PacketHandler/ServerHeartbeatPacketHandler.h"
#include "Enum.pb.h"
#include "Table/PlayerCharacterTable.h"
#include "Table/NavMeshSourceTable.h"
#include "PlayerObject.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

namespace
{
	bool IsS3Uri(const std::string& value)
	{
		return value.rfind("s3://", 0) == 0;
	}

	bool IsResolvedConfigValue(const std::string& value)
	{
		return !value.empty() && value.find("${") == std::string::npos;
	}

	std::string EscapeDoubleQuotes(const std::string& value)
	{
		std::string out;
		out.reserve(value.size());
		for (const char ch : value)
		{
			if (ch == '"')
			{
				out += "\\\"";
			}
			else
			{
				out.push_back(ch);
			}
		}
		return out;
	}

	std::string ResolveAwsCliExecutable()
	{
		char* envAwsPathRaw = nullptr;
		size_t envAwsPathLen = 0;
		if (_dupenv_s(&envAwsPathRaw, &envAwsPathLen, "DH1_AWS_CLI_PATH") == 0 && envAwsPathRaw != nullptr)
		{
			const std::string configuredPath(envAwsPathRaw);
			free(envAwsPathRaw);
			if (!configuredPath.empty() && std::filesystem::exists(configuredPath))
			{
				return configuredPath;
			}
		}

		constexpr const char* shortAwsPath = "C:\\Progra~1\\Amazon\\AWSCLIV2\\aws.exe";
		if (std::filesystem::exists(shortAwsPath))
		{
			return shortAwsPath;
		}

		constexpr const char* defaultAwsPath = "C:\\Program Files\\Amazon\\AWSCLIV2\\aws.exe";
		if (std::filesystem::exists(defaultAwsPath))
		{
			return defaultAwsPath;
		}

		return "aws";
	}

	std::string ResolveAwsCliProfileForS3()
	{
		const char* envNames[] = { "AWS_PROFILE", "DH1_AWS_PROFILE" };
		for (const char* name : envNames)
		{
			char* raw = nullptr;
			size_t len = 0;
			if (_dupenv_s(&raw, &len, name) != 0 || raw == nullptr)
			{
				continue;
			}
			std::string v(raw);
			free(raw);
			const size_t start = v.find_first_not_of(" \t\r\n");
			if (start == std::string::npos)
			{
				continue;
			}
			const size_t end = v.find_last_not_of(" \t\r\n");
			v = v.substr(start, end - start + 1);
			if (!v.empty())
			{
				return v;
			}
		}
		return {};
	}

	std::string ReadTextFileLimited(const std::string& path, const size_t maxBytes)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open())
		{
			return {};
		}

		std::ostringstream oss;
		oss << file.rdbuf();
		std::string text = oss.str();
		if (text.size() <= maxBytes)
		{
			return text;
		}

		return text.substr(text.size() - maxBytes);
	}

	bool DownloadS3UriToFile(const std::string& s3Uri, const std::string& targetPath, const std::string& s3Region,
		const int32 retryCount, const int32 retryDelayMs)
	{
		std::error_code ec;
		const std::filesystem::path outPath(targetPath);
		std::filesystem::create_directories(outPath.parent_path(), ec);
		if (ec)
		{
			NET_ENGINE_LOG_ERROR("WorldService::Initialize - Failed to create navmesh cache directory for S3 download: {}, ec={}",
				outPath.parent_path().string(), ec.value());
			return false;
		}

		const int32 attempts = std::max(1, retryCount);
		const std::string awsExe = ResolveAwsCliExecutable();
		const uint64 commandSeed = static_cast<uint64>(std::chrono::steady_clock::now().time_since_epoch().count());
		for (int32 attempt = 1; attempt <= attempts; ++attempt)
		{
			const std::filesystem::path commandLogPath = std::filesystem::temp_directory_path() /
				("dh1_world_aws_s3cp_" + std::to_string(commandSeed) + "_" + std::to_string(attempt) + ".log");

			std::string command = "cmd /c \"\"" + EscapeDoubleQuotes(awsExe) + "\" s3 cp \"" +
				EscapeDoubleQuotes(s3Uri) + "\" \"" + EscapeDoubleQuotes(targetPath) + "\" --only-show-errors";
			if (IsResolvedConfigValue(s3Region))
			{
				command += " --region \"" + EscapeDoubleQuotes(s3Region) + "\"";
			}
			const std::string awsProfile = ResolveAwsCliProfileForS3();
			if (!awsProfile.empty())
			{
				command += " --profile \"" + EscapeDoubleQuotes(awsProfile) + "\"";
			}
			command += " > \"" + EscapeDoubleQuotes(commandLogPath.string()) + "\" 2>&1\"";

			const int32 exitCode = std::system(command.c_str());
			if (exitCode == 0)
			{
				std::error_code removeEc;
				std::filesystem::remove(commandLogPath, removeEc);
				return true;
			}

			const std::string commandOutput = ReadTextFileLimited(commandLogPath.string(), 4096);
			std::error_code removeEc;
			std::filesystem::remove(commandLogPath, removeEc);

			NET_ENGINE_LOG_WARN(
				"WorldService::Initialize - S3 navmesh download failed, uri: {}, attempt: {}/{}, exitCode: {}, awsExe: {}, output: {}",
				s3Uri, attempt, attempts, exitCode, awsExe, commandOutput.empty() ? "<empty>" : commandOutput);

			if (attempt < attempts && retryDelayMs > 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
			}
		}

		return false;
	}
}

BOOL WINAPI WorldService::ConsoleCtrlHandler(const DWORD ctrlType)
{
	switch (ctrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
		GetInstance().Stop();
		return TRUE;
	default:
		return FALSE;
	}
}

bool WorldService::Initialize(const JsonConfig& config)
{
	const JsonConfig serverConfig = config.GetSection("server");
	const JsonConfig worldServerConfig = config.GetSection("worldServer");
	const JsonConfig sessionConfig = config.GetSection("session");
	const JsonConfig networkSchedulerConfig = config.GetSection("networkScheduler");
	const JsonConfig actorSchedulerConfig = config.GetSection("actorScheduler");

	// NetworkScheduler
	NetworkSchedulerConfig netConfig;
	netConfig.runningThreadCount = networkSchedulerConfig.GetUInt32("runningThreadCount");
	netConfig.waitTimeoutMs = networkSchedulerConfig.GetUInt32("waitTimeoutMs");
	netConfig.tickIntervalMs = networkSchedulerConfig.GetUInt32("tickIntervalMs");

	// ServerService (Listen for GatewayServer connections)
	mListenIp = serverConfig.GetString("ip");
	mListenPort = serverConfig.GetUInt16("port");

	const int32 receiveBufferSize = sessionConfig.GetInt32("receiveBufferSize");
	const int32 sendBufferSize = sessionConfig.GetInt32("sendBufferSize");

	ServerServiceConfig serviceConfig;
	serviceConfig.netAddress = NetAddress(mListenIp, mListenPort);
	serviceConfig.maxSessionCount = serverConfig.GetInt32("maxSessionCount");
	serviceConfig.acceptCount = serverConfig.GetInt32("acceptCount");
	serviceConfig.sessionTimeoutMs = serverConfig.GetInt64("sessionTimeoutMs");
	serviceConfig.pNetworkScheduler = cpp_net_engine::MakeShared<NetworkScheduler>(netConfig);
	serviceConfig.sessionFactory = [receiveBufferSize, sendBufferSize]() -> GatewaySessionRef
		{
			return cpp_net_engine::MakeShared<GatewaySession>(receiveBufferSize, sendBufferSize);
		};

	mpServerService = cpp_net_engine::MakeShared<ServerService>(eServiceType::Server);
	if (mpServerService->Initialize(serviceConfig) == false)
	{
		NET_ENGINE_LOG_ERROR("WorldService::Initialize - ServerService initialization failed");
		return false;
	}

	// ActorService
	ActorServiceConfig actorSvcConfig;
	actorSvcConfig.schedulerConfig.runningThreadCount = actorSchedulerConfig.GetUInt32("runningThreadCount");
	actorSvcConfig.schedulerConfig.waitTimeoutMs = actorSchedulerConfig.GetUInt32("waitTimeoutMs");
	actorSvcConfig.schedulerConfig.tickIntervalMs = actorSchedulerConfig.GetUInt32("tickIntervalMs");

	mpActorService = cpp_net_engine::MakeShared<ActorService>();
	if (mpActorService->Initialize(actorSvcConfig) == false)
	{
		NET_ENGINE_LOG_ERROR("WorldService::Initialize - ActorService initialization failed");
		return false;
	}

	// RedisService
	if (config.HasKey("redis"))
	{
		const JsonConfig redisConfig = config.GetSection("redis");
		mpRedisService = cpp_net_engine::MakeShared<RedisService>(redisConfig.GetString("connectionUri"), mpActorService);
	}

	// MySqlService
	if (config.HasKey("mysql"))
	{
		const JsonConfig mysqlConfig = config.GetSection("mysql");
		MySqlConfig dbConfig;
		dbConfig.host = mysqlConfig.GetString("host");
		dbConfig.port = mysqlConfig.GetUInt16("port");
		dbConfig.user = mysqlConfig.GetString("user");
		dbConfig.password = mysqlConfig.GetString("password");
		dbConfig.database = mysqlConfig.GetString("database");
		dbConfig.poolSize = mysqlConfig.HasKey("poolSize") ? mysqlConfig.GetInt32("poolSize") : 4;

		mpMySqlService = cpp_net_engine::MakeShared<MySqlService>(dbConfig, mpActorService);
	}

	// RealmServer ClientService — ServerService와 NetworkScheduler 공유
	if (config.HasKey("realmServer"))
	{
		const JsonConfig realmServerConfig = config.GetSection("realmServer");

		ClientServiceConfig realmServiceConfig;
		realmServiceConfig.netAddress = NetAddress(realmServerConfig.GetString("ip"), realmServerConfig.GetUInt16("port"));
		realmServiceConfig.maxSessionCount = 1;
		realmServiceConfig.maxConnectionCount = 1;
		realmServiceConfig.bAutoReconnect = true;
		realmServiceConfig.reconnectIntervalMs = realmServerConfig.GetInt64("reconnectIntervalMs");
		realmServiceConfig.maxReconnectCount = 0;
		realmServiceConfig.pNetworkScheduler = serviceConfig.pNetworkScheduler; // 스케줄러 공유
		realmServiceConfig.sessionFactory = [receiveBufferSize, sendBufferSize]() -> RealmSessionRef
			{
				return cpp_net_engine::MakeShared<RealmSession>(receiveBufferSize, sendBufferSize);
			};

		mpRealmClientService = cpp_net_engine::MakeShared<ClientService>(eServiceType::Client);
		if (mpRealmClientService->Initialize(realmServiceConfig) == false)
		{
			NET_ENGINE_LOG_ERROR("WorldService::Initialize - RealmServer ClientService initialization failed");
			return false;
		}
	}

	// Thread counts
	mNetworkDispatchThreadCount = networkSchedulerConfig.GetInt32("runningThreadCount");
	mActorDispatchThreadCount = actorSchedulerConfig.GetInt32("runningThreadCount");

	// WorldServer info
	mWorldServerId = worldServerConfig.GetInt32("worldServerId");
	mWorldName = worldServerConfig.GetString("worldName");
	mMaxPlayers = worldServerConfig.GetInt32("maxPlayers");
	mHeartbeatIntervalMs = worldServerConfig.GetInt64("heartbeatIntervalMs");
	mGatewayHeartbeatTimeoutMs = worldServerConfig.GetInt64("gatewayHeartbeatTimeoutMs");
	mRealmHeartbeatTimeoutMs = worldServerConfig.GetInt64("realmHeartbeatTimeoutMs");
	mRedisTtlSeconds = worldServerConfig.GetInt64("redisTtlSeconds");
	const std::string navMeshMapCode = worldServerConfig.HasKey("navMeshMapCode")
		? worldServerConfig.GetString("navMeshMapCode")
		: mWorldName;

	// GameTick config
	if (config.HasKey("gameTick"))
	{
		const JsonConfig gameTickConfig = config.GetSection("gameTick");
		mGameTickIntervalMs = gameTickConfig.GetInt64("tickIntervalMs");
		mDefaultMoveSpeed = static_cast<float>(gameTickConfig.GetInt32("moveSpeed"));
		mAoiCellSize = static_cast<float>(gameTickConfig.GetInt32("aoiCellSize"));
		mAoiRange = gameTickConfig.GetInt32("aoiRange");
	}

	// GameSessionManager
	mpGameSessionManager = cpp_net_engine::MakeShared<GameSessionManager>(mMaxPlayers);

	// GridManager
	mpGridManager = cpp_net_engine::MakeShared<GridManager>(mAoiCellSize, mAoiRange);

	// NavMeshManager
	mpNavMeshManager = cpp_net_engine::MakeShared<NavMeshManager>();
	if (config.HasKey("gameTick"))
	{
		const JsonConfig gameTickConfig = config.GetSection("gameTick");
		const bool navMeshRequireSuccess = gameTickConfig.GetBool("navMeshRequireSuccess", true);
		if (mpMySqlService == nullptr)
		{
			NET_ENGINE_LOG_ERROR("WorldService::Initialize - MySQL service is required for DB-backed NavMesh source");
			return false;
		}
		if (navMeshMapCode.empty())
		{
			NET_ENGINE_LOG_ERROR("WorldService::Initialize - worldServer.navMeshMapCode is required");
			return false;
		}

		mNavMeshFilePath.clear();
		try
		{
			mpMySqlService->ExecuteSync([this, &navMeshMapCode](sqlpp::mysql::connection& db)
				{
					const db::WorldNavmeshSource table{};
					const auto result = db(sqlpp::select(sqlpp::all_of(table))
						.from(table)
						.where((table.worldServerId == mWorldServerId)
							and (table.mapCode == navMeshMapCode)
							and (table.isActive == true)));

					if (!result.empty())
					{
						const auto& row = result.front();
						const std::string pathFromDb = row.navmeshPath;
						if (!pathFromDb.empty())
						{
							mNavMeshFilePath = pathFromDb;
						}
					}
				});
		}
		catch (const std::exception& ex)
		{
			NET_ENGINE_LOG_ERROR(
				"WorldService::Initialize - NavMesh DB lookup failed, worldServerId: {}, mapCode: {}, error: {}",
				mWorldServerId, navMeshMapCode, ex.what());
			return false;
		}

		if (mNavMeshFilePath.empty())
		{
			NET_ENGINE_LOG_ERROR(
				"WorldService::Initialize - NavMesh source is missing in DB, worldServerId: {}, mapCode: {}",
				mWorldServerId, navMeshMapCode);
			return false;
		}

		NET_ENGINE_LOG_INFO(
			"WorldService::Initialize - NavMesh source resolved from DB, worldServerId: {}, mapCode: {}, path: {}",
			mWorldServerId, navMeshMapCode, mNavMeshFilePath);

		std::string resolvedNavMeshPath = mNavMeshFilePath;
		if (!IsS3Uri(mNavMeshFilePath))
		{
			NET_ENGINE_LOG_ERROR(
				"WorldService::Initialize - Only S3 URI is supported for NavMesh source. path: {}",
				mNavMeshFilePath);
			return false;
		}

		if (gameTickConfig.HasKey("navMeshCacheFilePath"))
		{
			mNavMeshCacheFilePath = gameTickConfig.GetString("navMeshCacheFilePath");
		}

		if (!IsResolvedConfigValue(mNavMeshCacheFilePath))
		{
			const std::filesystem::path defaultCachePath =
				std::filesystem::temp_directory_path() / "DH1_MMORPG" / "NavMesh" /
				("world_" + std::to_string(mWorldServerId) + ".bin");
			mNavMeshCacheFilePath = defaultCachePath.string();
		}

		const std::string s3Region = gameTickConfig.HasKey("navMeshS3Region")
			? gameTickConfig.GetString("navMeshS3Region")
			: std::string();
		const int32 retryCount = gameTickConfig.HasKey("navMeshDownloadRetryCount")
			? gameTickConfig.GetInt32("navMeshDownloadRetryCount")
			: 3;
		const int32 retryDelayMs = gameTickConfig.HasKey("navMeshDownloadRetryDelayMs")
			? gameTickConfig.GetInt32("navMeshDownloadRetryDelayMs")
			: 500;

		if (DownloadS3UriToFile(mNavMeshFilePath, mNavMeshCacheFilePath, s3Region, retryCount, retryDelayMs))
		{
			resolvedNavMeshPath = mNavMeshCacheFilePath;
			NET_ENGINE_LOG_INFO("WorldService::Initialize - NavMesh downloaded from S3 URI to cache path: {}",
				resolvedNavMeshPath);
		}
		else
		{
			NET_ENGINE_LOG_ERROR("WorldService::Initialize - Failed to download NavMesh from S3 URI, source: {}",
				mNavMeshFilePath);
			if (navMeshRequireSuccess)
			{
				return false;
			}

			NET_ENGINE_LOG_WARN("WorldService::Initialize - NavMesh requireSuccess disabled, pathfinding disabled");
			resolvedNavMeshPath.clear();
		}

		if (resolvedNavMeshPath.empty() || !IsResolvedConfigValue(resolvedNavMeshPath))
		{
			NET_ENGINE_LOG_ERROR("WorldService::Initialize - NavMesh path is empty or unresolved: {}", resolvedNavMeshPath);
			if (navMeshRequireSuccess)
			{
				return false;
			}
			NET_ENGINE_LOG_WARN("WorldService::Initialize - NavMesh requireSuccess disabled, pathfinding disabled");
		}
		else if (!mpNavMeshManager->LoadFromFile(resolvedNavMeshPath))
		{
			NET_ENGINE_LOG_ERROR("WorldService::Initialize - NavMesh load failed ({})", resolvedNavMeshPath);
			if (navMeshRequireSuccess)
			{
				return false;
			}
			NET_ENGINE_LOG_WARN("WorldService::Initialize - NavMesh requireSuccess disabled, pathfinding disabled");
		}
	}
	else
	{
		NET_ENGINE_LOG_ERROR("WorldService::Initialize - gameTick section is required");
		return false;
	}

	// GameTickProcessor
	mpGameTickProcessor = cpp_net_engine::MakeShared<GameTickProcessor>(mpGridManager, mpNavMeshManager, mDefaultMoveSpeed);

	NET_ENGINE_LOG_INFO("WorldService::Initialize - Initialization complete, worldServerId: {}, tickIntervalMs: {}", mWorldServerId, mGameTickIntervalMs);
	return true;
}

bool WorldService::Start()
{
	mpActorService->Start();

	if (mpServerService->Start() == false)
	{
		NET_ENGINE_LOG_ERROR("WorldService::Start - ServerService start failed");
		return false;
	}

	// RealmServer connection (NetworkScheduler는 ServerService와 공유)
	if (mpRealmClientService != nullptr)
	{
		if (mpRealmClientService->Start() == false)
		{
			NET_ENGINE_LOG_ERROR("WorldService::Start - RealmServer ClientService start failed");
			return false;
		}
	}

	// Network dispatch threads (ServerService + RealmClientService 공유 스케줄러)
	for (int32 iter = 0; iter < mNetworkDispatchThreadCount; ++iter)
	{
		ThreadManager::GetInstance().Launch("Network Dispatch", [pScheduler = mpServerService->GetNetworkScheduler()]() -> void
			{
				while (!pScheduler->IsStopped())
				{
					pScheduler->Dispatch();
				}
			});
	}

	// Actor dispatch threads
	for (int32 iter = 0; iter < mActorDispatchThreadCount; ++iter)
	{
		ThreadManager::GetInstance().Launch("Actor Dispatch", [pActorService = mpActorService]() -> void
			{
				const auto pScheduler = pActorService->GetActorScheduler();
				while (!pScheduler->IsStopped())
				{
					pActorService->Dispatch();
				}
			});
	}

	mbRunning.store(true);

	NET_ENGINE_LOG_INFO("WorldService::Start - Server started, worldServerId: {}, port: {}", mWorldServerId, mListenPort);
	return true;
}

void WorldService::Run()
{
	updateWorldRegistration(eWorldServerStatus::Online);

	auto lastHeartbeat = std::chrono::steady_clock::now();
	auto lastGameTick = std::chrono::steady_clock::now();

	while (mbRunning.load())
	{
		const auto now = std::chrono::steady_clock::now();

		// 게임 틱 (20Hz, 50ms)
		const auto gameTickElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastGameTick).count();
		if (gameTickElapsedMs >= mGameTickIntervalMs)
		{
			mpGameTickProcessor->ProcessTick(gameTickElapsedMs);
			lastGameTick = now;
		}

		// 하트비트 (5000ms)
		const auto heartbeatElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeat).count();
		if (heartbeatElapsedMs >= mHeartbeatIntervalMs)
		{
			updateWorldRegistration(eWorldServerStatus::Online);
			sendHeartbeatToRealm();
			sendHeartbeatToGateways();
			checkGatewayHeartbeats();
			checkRealmHeartbeat();
			cleanupPendingSessions();
			lastHeartbeat = now;

			NET_ENGINE_LOG_INFO("WorldService::Run - SessionCount: {}, PlayerCount: {}, WorldServerId: {}",
				mpServerService->GetCurrentSessionCount(), mpGridManager->GetObjectCount(), mWorldServerId);
		}


		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	updateWorldRegistration(eWorldServerStatus::ShuttingDown);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	NET_ENGINE_LOG_INFO("WorldService::Run - Closing service...");

	if (mpRealmClientService != nullptr)
	{
		mpRealmClientService->CloseService(1);
	}

	mpServerService->CloseService(mNetworkDispatchThreadCount);
	mpActorService->CloseService(mActorDispatchThreadCount);

	ThreadManager::GetInstance().JoinWithClear();

	NET_ENGINE_LOG_INFO("WorldService::Run - Shutdown complete");
}

void WorldService::Stop()
{
	mbRunning.store(false);
}

ServerServiceRef WorldService::GetServerServiceRef() const
{
	return mpServerService;
}

ClientServiceRef WorldService::GetRealmClientServiceRef() const
{
	return mpRealmClientService;
}

ActorServiceRef WorldService::GetActorServiceRef() const
{
	return mpActorService;
}

RedisServiceRef WorldService::GetRedisServiceRef() const
{
	return mpRedisService;
}

MySqlServiceRef WorldService::GetMySqlServiceRef() const
{
	return mpMySqlService;
}

int32 WorldService::GetWorldServerId() const
{
	return mWorldServerId;
}

GameSessionManagerRef WorldService::GetGameSessionManagerRef() const
{
	return mpGameSessionManager;
}

GridManagerRef WorldService::GetGridManagerRef() const
{
	return mpGridManager;
}

GameTickProcessorRef WorldService::GetGameTickProcessorRef() const
{
	return mpGameTickProcessor;
}

NavMeshManagerRef WorldService::GetNavMeshManagerRef() const
{
	return mpNavMeshManager;
}

float WorldService::GetDefaultMoveSpeed() const
{
	return mDefaultMoveSpeed;
}

void WorldService::sendHeartbeatToRealm()
{
	if (mpRealmClientService == nullptr)
	{
		return;
	}

	const SessionRef pSession = mpRealmClientService->GetFirstSessionRef();
	if (pSession == nullptr)
	{
		return;
	}

	Protocol::S2S_HEARTBEAT_NOT packet;
	packet.set_servertype(static_cast<int32>(Protocol::eRole::WORLD_SERVER));
	packet.set_serverid(mWorldServerId);
	packet.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count());
	packet.set_sessioncount(mpServerService->GetCurrentSessionCount());
	packet.set_status(static_cast<int32>(eWorldServerStatus::Online));

	pSession->Send(ServerHeartbeatPacketHandler::MakeSendBuffer(packet));
}

void WorldService::sendHeartbeatToGateways()
{
	const Vector<SessionRef> sessions = mpServerService->GetActiveSessions();

	Protocol::S2S_HEARTBEAT_NOT packet;
	packet.set_servertype(static_cast<int32>(Protocol::eRole::WORLD_SERVER));
	packet.set_serverid(mWorldServerId);
	packet.set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count());
	packet.set_sessioncount(mpServerService->GetCurrentSessionCount());
	packet.set_status(static_cast<int32>(eWorldServerStatus::Online));

	const auto sendBuffer = ServerHeartbeatPacketHandler::MakeSendBuffer(packet);

	for (const auto& pSession : sessions)
	{
		pSession->Send(sendBuffer);
	}
}

void WorldService::checkGatewayHeartbeats()
{
	const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	const Vector<SessionRef> sessions = mpServerService->GetActiveSessions();

	for (const auto& pSession : sessions)
	{
		const auto pGatewaySession = std::static_pointer_cast<GatewaySession>(pSession);
		if (pGatewaySession == nullptr)
		{
			continue;
		}

		const int64 lastHeartbeatMs = pGatewaySession->GetLastHeartbeatMs();
		if (lastHeartbeatMs == 0)
		{
			continue;
		}

		const int64 elapsedMs = nowMs - lastHeartbeatMs;
		if (elapsedMs > mGatewayHeartbeatTimeoutMs)
		{
			NET_ENGINE_LOG_WARN("WorldService::checkGatewayHeartbeats - GatewayServer heartbeat timeout, "
				"gatewayServerId: {}, elapsedMs: {}", pGatewaySession->GetGatewayServerId(), elapsedMs);
			pGatewaySession->Disconnect(eDisconnectReason::Timeout);
		}
	}
}

void WorldService::checkRealmHeartbeat()
{
	if (mpRealmClientService == nullptr)
	{
		return;
	}

	const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();

	const SessionRef pSession = mpRealmClientService->GetFirstSessionRef();
	if (pSession == nullptr)
	{
		return;
	}

	const auto pRealmSession = std::static_pointer_cast<RealmSession>(pSession);
	if (pRealmSession == nullptr)
	{
		return;
	}

	const int64 lastHeartbeatMs = pRealmSession->GetLastHeartbeatMs();
	if (lastHeartbeatMs == 0)
	{
		return;
	}

	const int64 elapsedMs = nowMs - lastHeartbeatMs;
	if (elapsedMs > mRealmHeartbeatTimeoutMs)
	{
		NET_ENGINE_LOG_WARN("WorldService::checkRealmHeartbeat - RealmServer heartbeat timeout, elapsedMs: {}", elapsedMs);
		pRealmSession->Disconnect(eDisconnectReason::Timeout);
	}
}

void WorldService::updateWorldRegistration(const eWorldServerStatus status)
{
	if (mpRedisService == nullptr)
	{
		return;
	}

	RedisWorldRegistration info;
	info.worldId = mWorldServerId;
	info.worldName = mWorldName;
	info.currentPlayers = mpServerService->GetCurrentSessionCount();
	info.maxPlayers = mMaxPlayers;
	info.status = static_cast<int32>(status);

	mpRedisService->UpdateWorldServerInfo(info, mRedisTtlSeconds);
}


void WorldService::cleanupPendingSessions()
{
	constexpr int64 pendingTimeoutMs = 30000; // 30초

	const Vector<uint64> expiredAccountIds = mpGameSessionManager->GetExpiredPendingSessions(pendingTimeoutMs);

	for (const uint64 accountId : expiredAccountIds)
	{
		NET_ENGINE_LOG_WARN("WorldService::cleanupPendingSessions - Pending session expired, accountId: {}", accountId);

		// PlayerObject 제거 (주변 플레이어에게 LEAVE 전송 후)
		if (mpGridManager != nullptr)
		{
			const auto pObject = mpGridManager->GetObjectByAccountId(accountId);
			if (pObject != nullptr)
			{
				if (mpGameTickProcessor != nullptr)
				{
					mpGameTickProcessor->NotifyNearbyPlayersAboutEntityLeave(pObject);
				}
				mpGridManager->RemoveObject(pObject->GetId());
			}
		}

		// 세션 제거
		(void)mpGameSessionManager->RemoveSession(accountId);

		// RealmServer에 이탈 동기화
		if (mpRealmClientService != nullptr)
		{
			const auto pRealmSession = std::static_pointer_cast<RealmSession>(mpRealmClientService->GetFirstSessionRef());
			if (pRealmSession != nullptr)
			{
				Protocol::S2S_GAME_SESSION_SYNC_LEAVE_NOT syncPacket;
				syncPacket.set_accountid(accountId);
				syncPacket.set_worldserverid(mWorldServerId);

				pRealmSession->Send(GameSessionPacketHandler::MakeSendBuffer(syncPacket));
			}
		}
	}
}
