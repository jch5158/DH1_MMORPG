using LoginServer.Controllers;
using LoginServer.Data;
using LoginServer.Data.Table;
using LoginServer.Services;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Logging;
using Moq;
using StackExchange.Redis;
using System.Net;

namespace LoginServer.Tests;

public abstract class AuthControllerTestBase : IDisposable
{
    protected const string TestPassword = "TestPass123!";
    protected static readonly string TestPasswordHash = BCrypt.Net.BCrypt.HashPassword(TestPassword);

    protected readonly AccountDbContext DbContext;
    protected readonly Mock<IConnectionMultiplexer> MockRedisConnection;
    protected readonly Mock<IDatabase> MockRedisDb;
    protected readonly Mock<IServer> MockRedisServer;
    protected readonly Mock<IEmailQueue> MockEmailQueue;

    protected AuthControllerTestBase()
    {
        var options = new DbContextOptionsBuilder<AccountDbContext>()
            .UseInMemoryDatabase(Guid.NewGuid().ToString())
            .Options;
        DbContext = new AccountDbContext(options);

        MockRedisDb = new Mock<IDatabase>(MockBehavior.Loose);
        MockRedisServer = new Mock<IServer>();
        MockRedisConnection = new Mock<IConnectionMultiplexer>();

        var endpoint = new DnsEndPoint("localhost", 6379);
        MockRedisConnection.Setup(c => c.GetDatabase(It.IsAny<int>(), It.IsAny<object>()))
            .Returns(MockRedisDb.Object);
        MockRedisConnection.Setup(c => c.GetEndPoints(It.IsAny<bool>()))
            .Returns([endpoint]);
        MockRedisConnection.Setup(c => c.GetServer(It.IsAny<EndPoint>(), It.IsAny<object>()))
            .Returns(MockRedisServer.Object);

        MockEmailQueue = new Mock<IEmailQueue>();
        MockEmailQueue.Setup(e => e.QueueEmailAsync(It.IsAny<EmailJob>()))
            .Returns(ValueTask.CompletedTask);
    }

    protected AuthController CreateController()
    {
        var gatewaySelector = new GatewaySelector(MockRedisConnection.Object, new Mock<ILogger<GatewaySelector>>().Object);
        var authService = new AuthService(DbContext, MockRedisConnection.Object, MockEmailQueue.Object, gatewaySelector, new Mock<ILogger<AuthService>>().Object);
        var controller = new AuthController(authService);
        controller.ControllerContext = new ControllerContext
        {
            HttpContext = new DefaultHttpContext()
        };
        return controller;
    }

    protected async Task<Account> SeedAccount(
        string email = "test@example.com",
        string? passwordHash = null,
        EAccountState state = EAccountState.Active)
    {
        var account = new Account
        {
            Email = email,
            PasswordHash = passwordHash ?? TestPasswordHash,
            AccountState = state
        };
        DbContext.Accounts.Add(account);
        await DbContext.SaveChangesAsync();
        return account;
    }

    protected void SetupGatewayKeys(params (string ip, int port, int status, int sessionCount)[] gateways)
    {
        var keys = new List<RedisKey>();
        for (var i = 0; i < gateways.Length; i++)
        {
            var key = new RedisKey($"Gateway:{i + 1}");
            keys.Add(key);

            var (ip, port, status, sessionCount) = gateways[i];
            var json = $"{{\"gatewayId\":{i + 1},\"ip\":\"{ip}\",\"port\":{port},\"status\":{status},\"currentSessionCount\":{sessionCount}}}";
            MockRedisDb.Setup(d => d.StringGetAsync(key, It.IsAny<CommandFlags>()))
                .ReturnsAsync(new RedisValue(json));
        }

        MockRedisServer.Setup(s => s.Keys(It.IsAny<int>(), It.IsAny<RedisValue>(), It.IsAny<int>(), It.IsAny<long>(), It.IsAny<int>(), It.IsAny<CommandFlags>()))
            .Returns(keys);
    }

    protected void SetupNoGateways()
    {
        MockRedisServer.Setup(s => s.Keys(It.IsAny<int>(), It.IsAny<RedisValue>(), It.IsAny<int>(), It.IsAny<long>(), It.IsAny<int>(), It.IsAny<CommandFlags>()))
            .Returns(Array.Empty<RedisKey>());
    }

    protected void SetupRedisKeyExists(string key, bool exists)
    {
        MockRedisDb.Setup(d => d.KeyExistsAsync(new RedisKey(key), It.IsAny<CommandFlags>()))
            .ReturnsAsync(exists);
    }

    protected void SetupRedisStringGet(string key, string? value)
    {
        MockRedisDb.Setup(d => d.StringGetAsync(new RedisKey(key), It.IsAny<CommandFlags>()))
            .ReturnsAsync(value != null ? new RedisValue(value) : RedisValue.Null);
    }

    protected void SetupRedisStringSet(bool result = true)
    {
        // Use DefaultValue to return true for any unmatched Task<bool> calls
        MockRedisDb.SetReturnsDefault(Task.FromResult(result));
    }

    protected void SetupRedisStringIncrement(string key, long returnValue)
    {
        MockRedisDb.Setup(d => d.StringIncrementAsync(new RedisKey(key), It.IsAny<long>(), It.IsAny<CommandFlags>()))
            .ReturnsAsync(returnValue);
    }

    protected static int GetStatusCode(IActionResult result)
    {
        return result switch
        {
            ObjectResult obj => obj.StatusCode ?? 200,
            StatusCodeResult status => status.StatusCode,
            _ => 200
        };
    }

    protected static object? GetResponseValue(IActionResult result)
    {
        return result is ObjectResult obj ? obj.Value : null;
    }

    protected static string? GetResponseProperty(IActionResult result, string propertyName)
    {
        var value = GetResponseValue(result);
        if (value == null) return null;
        var prop = value.GetType().GetProperty(propertyName);
        return prop?.GetValue(value)?.ToString();
    }

    public void Dispose()
    {
        DbContext.Dispose();
        GC.SuppressFinalize(this);
    }
}
