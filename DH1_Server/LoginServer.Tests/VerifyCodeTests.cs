using LoginServer.Data.Table;
using LoginServer.DTOs.Auth;
using Microsoft.EntityFrameworkCore;
using Moq;
using StackExchange.Redis;

namespace LoginServer.Tests;

public class VerifyCodeTests : AuthControllerTestBase
{
    private const string TestEmail = "verify@example.com";
    private const string TestCode = "123456";

    [Fact]
    public async Task VerifyCode_Correct_ActivatesAccount()
    {
        await SeedAccount(TestEmail, state: EAccountState.EmailUnverified);
        SetupRedisStringGet($"VerifyCode_{TestEmail}", TestCode);

        var controller = CreateController();
        var result = await controller.CompareVerifyAuthCode(new VerifyCodeRequest { Email = TestEmail, VerifyCode = TestCode });

        Assert.Equal(200, GetStatusCode(result));

        var account = await DbContext.Accounts.SingleAsync(a => a.Email == TestEmail);
        Assert.Equal(EAccountState.Active, account.AccountState);
    }

    [Fact]
    public async Task VerifyCode_WrongCode_Returns400()
    {
        await SeedAccount(TestEmail, state: EAccountState.EmailUnverified);
        SetupRedisStringGet($"VerifyCode_{TestEmail}", "654321");

        var controller = CreateController();
        var result = await controller.CompareVerifyAuthCode(new VerifyCodeRequest { Email = TestEmail, VerifyCode = TestCode });

        Assert.Equal(400, GetStatusCode(result));
    }

    [Fact]
    public async Task VerifyCode_Expired_Returns400()
    {
        await SeedAccount(TestEmail, state: EAccountState.EmailUnverified);
        SetupRedisStringGet($"VerifyCode_{TestEmail}", null);

        var controller = CreateController();
        var result = await controller.CompareVerifyAuthCode(new VerifyCodeRequest { Email = TestEmail, VerifyCode = TestCode });

        Assert.Equal(400, GetStatusCode(result));
    }

    [Fact]
    public async Task VerifyCode_AlreadyActive_Returns400()
    {
        await SeedAccount(TestEmail, state: EAccountState.Active);
        SetupRedisStringGet($"VerifyCode_{TestEmail}", TestCode);

        var controller = CreateController();
        var result = await controller.CompareVerifyAuthCode(new VerifyCodeRequest { Email = TestEmail, VerifyCode = TestCode });

        Assert.Equal(400, GetStatusCode(result));
    }

    [Fact]
    public async Task VerifyCode_CleansUpRedis()
    {
        await SeedAccount(TestEmail, state: EAccountState.EmailUnverified);
        SetupRedisStringGet($"VerifyCode_{TestEmail}", TestCode);

        var controller = CreateController();
        await controller.CompareVerifyAuthCode(new VerifyCodeRequest { Email = TestEmail, VerifyCode = TestCode });

        MockRedisDb.Verify(d => d.KeyDeleteAsync(new RedisKey($"VerifyCode_{TestEmail}"), It.IsAny<CommandFlags>()), Times.Once);
        MockRedisDb.Verify(d => d.KeyDeleteAsync(new RedisKey($"Cooldown_{TestEmail}"), It.IsAny<CommandFlags>()), Times.Once);
    }
}
