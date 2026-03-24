using StackExchange.Redis;
using System.Text.Json;

namespace LoginServer.Services
{
    public class GatewayInfo
    {
        public string Ip { get; set; } = string.Empty;
        public int Port { get; set; }
        public int CurrentSessionCount { get; set; }
        public int Status { get; set; }
    }

    public class GatewaySelector(IConnectionMultiplexer redisConnection, ILogger<GatewaySelector> logger)
    {
        private const int StatusOnline = 0;

        public async Task<(string ip, int port)?> SelectBestGatewayAsync()
        {
            var redisDb = redisConnection.GetDatabase();
            var server = redisConnection.GetServer(redisConnection.GetEndPoints().First());

            var gatewayKeys = server.Keys(pattern: "Gateway:*").ToArray();
            if (gatewayKeys.Length == 0)
            {
                logger.LogWarning("No gateway servers registered in Redis");
                return null;
            }

            string? bestIp = null;
            var bestPort = 0;
            var lowestCount = int.MaxValue;

            foreach (var key in gatewayKeys)
            {
                var value = await redisDb.StringGetAsync(key);
                if (!value.HasValue)
                {
                    continue;
                }

                try
                {
                    var info = JsonSerializer.Deserialize<JsonElement>(value.ToString());
                    var status = info.GetProperty("status").GetInt32();
                    if (status != StatusOnline)
                    {
                        continue;
                    }

                    var sessionCount = info.GetProperty("currentSessionCount").GetInt32();
                    if (sessionCount < lowestCount)
                    {
                        lowestCount = sessionCount;
                        bestIp = info.GetProperty("ip").GetString();
                        bestPort = info.GetProperty("port").GetInt32();
                    }
                }
                catch (Exception ex)
                {
                    logger.LogWarning(ex, "Failed to parse gateway info for key {Key}", key);
                }
            }

            if (bestIp == null)
            {
                logger.LogWarning("No online gateway servers available");
                return null;
            }

            logger.LogInformation("Selected gateway {Ip}:{Port} with {SessionCount} sessions", bestIp, bestPort, lowestCount);
            return (bestIp, bestPort);
        }
    }
}
