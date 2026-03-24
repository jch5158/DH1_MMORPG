using LoginServer.Data.Table;
using LoginServer.DTOs.Auth;
using LoginServer.Services;
using Moq;
using StackExchange.Redis;

namespace LoginServer.Tests;

public class SendVerifyCodeTests : AuthControllerTestBase
{
    private const string TestEmail = "sendcode@example.com";

    [Fact]
    public async Task SendVerifyCode_UnverifiedAccount_SendsEmail()
    {
        await SeedAccount(TestEmail, state: EAccountState.EmailUnverified);
        SetupRedisKeyExists($"Cooldown_{TestEmail}", false);
        SetupRedisStringSet();

        var controller = CreateController();
        var result = await controller.SendVerifyCode(new SendVerifyCodeRequest { Email = TestEmail });

        Assert.Equal(200, GetStatusCode(result));
        MockEmailQueue.Verify(e => e.QueueEmailAsync(It.Is<EmailJob>(j => j.ToEmail == TestEmail)), Times.Once);
    }

    [Fact]
    public async Task SendVerifyCode_ActiveAccount_Returns409()
    {
        await SeedAccount(TestEmail, state: EAccountState.Active);

        var controller = CreateController();
        var result = await controller.SendVerifyCode(new SendVerifyCodeRequest { Email = TestEmail });

        Assert.Equal(409, GetStatusCode(result));
    }

    [Fact]
    public async Task SendVerifyCode_NonExistent_Returns404()
    {
        var controller = CreateController();
        var result = await controller.SendVerifyCode(new SendVerifyCodeRequest { Email = "nobody@example.com" });

        Assert.Equal(404, GetStatusCode(result));
    }

    [Fact]
    public async Task SendVerifyCode_Cooldown_Returns400()
    {
        await SeedAccount(TestEmail, state: EAccountState.EmailUnverified);
        SetupRedisKeyExists($"Cooldown_{TestEmail}", true);

        var controller = CreateController();
        var result = await controller.SendVerifyCode(new SendVerifyCodeRequest { Email = TestEmail });

        Assert.Equal(400, GetStatusCode(result));
    }
}
