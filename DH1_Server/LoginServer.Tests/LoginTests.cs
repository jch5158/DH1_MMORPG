using LoginServer.Data.Table;
using LoginServer.DTOs.Auth;
using LoginServer.Services;
using Moq;
using StackExchange.Redis;

namespace LoginServer.Tests;

public class LoginTests : AuthControllerTestBase
{
    private const string TestEmail = "login@example.com";
    private readonly LoginRequest _validRequest = new() { Email = TestEmail, Password = TestPassword };

    private void SetupDefaultLoginMocks(bool locked = false, long failCount = 0)
    {
        SetupRedisKeyExists($"LoginLock_{TestEmail}", locked);

        if (!locked)
        {
            SetupRedisStringIncrement($"LoginFail_{TestEmail}", failCount + 1);
            SetupRedisStringSet();

            MockRedisDb.Setup(d => d.KeyExpireAsync(It.IsAny<RedisKey>(), It.IsAny<TimeSpan?>(), It.IsAny<ExpireWhen>(), It.IsAny<CommandFlags>()))
                .ReturnsAsync(true);
            MockRedisDb.Setup(d => d.KeyDeleteAsync(It.IsAny<RedisKey>(), It.IsAny<CommandFlags>()))
                .ReturnsAsync(true);
        }
    }

    [Fact]
    public async Task Login_ValidCredentials_ReturnsTicketAndGateway()
    {
        await SeedAccount(TestEmail);
        SetupDefaultLoginMocks();
        SetupGatewayKeys(("127.0.0.1", 9000, 0, 10));

        var controller = CreateController();
        var result = await controller.Login(_validRequest);

        Assert.Equal(200, GetStatusCode(result));
        Assert.NotNull(GetResponseProperty(result, "ticket"));
        Assert.Equal("127.0.0.1", GetResponseProperty(result, "gatewayIp"));
        Assert.Equal("9000", GetResponseProperty(result, "gatewayPort"));
    }

    [Fact]
    public async Task Login_InvalidEmail_Returns400()
    {
        SetupDefaultLoginMocks();

        var controller = CreateController();
        var result = await controller.Login(_validRequest);

        Assert.Equal(400, GetStatusCode(result));
    }

    [Fact]
    public async Task Login_WrongPassword_Returns400_IncrementsFailCount()
    {
        await SeedAccount(TestEmail);
        SetupDefaultLoginMocks(failCount: 0);

        var controller = CreateController();
        var result = await controller.Login(new LoginRequest { Email = TestEmail, Password = "WrongPass123!" });

        Assert.Equal(400, GetStatusCode(result));
        MockRedisDb.Verify(
            d => d.StringIncrementAsync(new RedisKey($"LoginFail_{TestEmail}"), It.IsAny<long>(), It.IsAny<CommandFlags>()),
            Times.Once);
    }

    [Fact]
    public async Task Login_5thFailedAttempt_Returns429_SetsLock()
    {
        await SeedAccount(TestEmail);
        SetupDefaultLoginMocks(failCount: 4);

        var controller = CreateController();
        var result = await controller.Login(new LoginRequest { Email = TestEmail, Password = "WrongPass123!" });

        Assert.Equal(429, GetStatusCode(result));
    }

    [Fact]
    public async Task Login_LockedAccount_Returns429()
    {
        SetupDefaultLoginMocks(locked: true);

        var controller = CreateController();
        var result = await controller.Login(_validRequest);

        Assert.Equal(429, GetStatusCode(result));
    }

    [Fact]
    public async Task Login_SuspendedAccount_Returns403()
    {
        await SeedAccount(TestEmail, state: EAccountState.Suspended);
        SetupDefaultLoginMocks();
        SetupGatewayKeys(("127.0.0.1", 9000, 0, 5));

        var controller = CreateController();
        var result = await controller.Login(_validRequest);

        Assert.Equal(403, GetStatusCode(result));
        Assert.Equal("ACCOUNT_SUSPENDED", GetResponseProperty(result, "code"));
    }

    [Fact]
    public async Task Login_BannedAccount_Returns403()
    {
        await SeedAccount(TestEmail, state: EAccountState.Banned);
        SetupDefaultLoginMocks();
        SetupGatewayKeys(("127.0.0.1", 9000, 0, 5));

        var controller = CreateController();
        var result = await controller.Login(_validRequest);

        Assert.Equal(403, GetStatusCode(result));
        Assert.Equal("ACCOUNT_BANNED", GetResponseProperty(result, "code"));
    }

    [Fact]
    public async Task Login_EmailUnverified_Returns403()
    {
        await SeedAccount(TestEmail, state: EAccountState.EmailUnverified);
        SetupDefaultLoginMocks();
        SetupGatewayKeys(("127.0.0.1", 9000, 0, 5));

        var controller = CreateController();
        var result = await controller.Login(_validRequest);

        Assert.Equal(403, GetStatusCode(result));
        Assert.Equal("EMAIL_UNVERIFIED", GetResponseProperty(result, "code"));
    }

    [Fact]
    public async Task Login_PendingUnregister_Returns403()
    {
        await SeedAccount(TestEmail, state: EAccountState.PendingUnregister);
        SetupDefaultLoginMocks();
        SetupGatewayKeys(("127.0.0.1", 9000, 0, 5));

        var controller = CreateController();
        var result = await controller.Login(_validRequest);

        Assert.Equal(403, GetStatusCode(result));
        Assert.Equal("ACCOUNT_PENDING_UNREGISTER", GetResponseProperty(result, "code"));
    }

    [Fact]
    public async Task Login_TicketSetFails_Returns500()
    {
        await SeedAccount(TestEmail);
        SetupDefaultLoginMocks();
        // Override default true → false for all Task<bool> returns
        MockRedisDb.SetReturnsDefault(Task.FromResult(false));
        SetupGatewayKeys(("127.0.0.1", 9000, 0, 5));

        var controller = CreateController();
        var result = await controller.Login(_validRequest);

        Assert.Equal(500, GetStatusCode(result));
    }

    [Fact]
    public async Task Login_NoGateway_Returns503()
    {
        await SeedAccount(TestEmail);
        SetupDefaultLoginMocks();
        SetupNoGateways();

        var controller = CreateController();
        var result = await controller.Login(_validRequest);

        Assert.Equal(503, GetStatusCode(result));
    }

    [Fact]
    public async Task Login_MultipleGateways_SelectsLowestSessionCount()
    {
        await SeedAccount(TestEmail);
        SetupDefaultLoginMocks();
        SetupGatewayKeys(
            ("10.0.0.1", 9001, 0, 100),   // Online, 100 sessions
            ("10.0.0.2", 9002, 1, 5),      // Maintenance, 5 sessions (should be skipped)
            ("10.0.0.3", 9003, 0, 20)      // Online, 20 sessions (should be selected)
        );

        var controller = CreateController();
        var result = await controller.Login(_validRequest);

        Assert.Equal(200, GetStatusCode(result));
        Assert.Equal("10.0.0.3", GetResponseProperty(result, "gatewayIp"));
        Assert.Equal("9003", GetResponseProperty(result, "gatewayPort"));
    }

    [Fact]
    public async Task Login_Success_ResetsFailCount()
    {
        await SeedAccount(TestEmail);
        SetupDefaultLoginMocks();
        SetupGatewayKeys(("127.0.0.1", 9000, 0, 5));

        var controller = CreateController();
        await controller.Login(_validRequest);

        MockRedisDb.Verify(
            d => d.KeyDeleteAsync(new RedisKey($"LoginFail_{TestEmail}"), It.IsAny<CommandFlags>()),
            Times.Once);
    }
}
