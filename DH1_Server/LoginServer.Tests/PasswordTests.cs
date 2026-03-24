using LoginServer.Data.Table;
using LoginServer.DTOs.Auth;
using LoginServer.Services;
using Microsoft.EntityFrameworkCore;
using Moq;
using StackExchange.Redis;

namespace LoginServer.Tests;

public class PasswordTests : AuthControllerTestBase
{
    private const string TestEmail = "password@example.com";
    private const string ResetCode = "654321";

    #region ForgotPassword

    [Fact]
    public async Task ForgotPassword_ExistingAccount_SendsEmail()
    {
        await SeedAccount(TestEmail, state: EAccountState.Active);
        SetupRedisKeyExists($"ResetCooldown_{TestEmail}", false);
        SetupRedisStringSet();

        var controller = CreateController();
        var result = await controller.ForgotPassword(new ForgotPasswordRequest { Email = TestEmail });

        Assert.Equal(200, GetStatusCode(result));
        MockEmailQueue.Verify(e => e.QueueEmailAsync(It.Is<EmailJob>(j => j.ToEmail == TestEmail)), Times.Once);
    }

    [Fact]
    public async Task ForgotPassword_NonExistentEmail_ReturnsOk()
    {
        var controller = CreateController();
        var result = await controller.ForgotPassword(new ForgotPasswordRequest { Email = "nobody@example.com" });

        Assert.Equal(200, GetStatusCode(result));
        MockEmailQueue.Verify(e => e.QueueEmailAsync(It.IsAny<EmailJob>()), Times.Never);
    }

    [Fact]
    public async Task ForgotPassword_UnverifiedAccount_Returns400()
    {
        await SeedAccount(TestEmail, state: EAccountState.EmailUnverified);

        var controller = CreateController();
        var result = await controller.ForgotPassword(new ForgotPasswordRequest { Email = TestEmail });

        Assert.Equal(400, GetStatusCode(result));
    }

    [Fact]
    public async Task ForgotPassword_Cooldown_Returns400()
    {
        await SeedAccount(TestEmail, state: EAccountState.Active);
        SetupRedisKeyExists($"ResetCooldown_{TestEmail}", true);

        var controller = CreateController();
        var result = await controller.ForgotPassword(new ForgotPasswordRequest { Email = TestEmail });

        Assert.Equal(400, GetStatusCode(result));
    }

    #endregion

    #region ResetPassword

    [Fact]
    public async Task ResetPassword_CorrectCode_ChangesPassword()
    {
        await SeedAccount(TestEmail, state: EAccountState.Active);
        SetupRedisStringGet($"ResetCode_{TestEmail}", ResetCode);

        var controller = CreateController();
        var result = await controller.ResetPassword(new ResetPasswordRequest
        {
            Email = TestEmail, VerifyCode = ResetCode, ResetPassword = "NewPassword1!"
        });

        Assert.Equal(200, GetStatusCode(result));

        var account = await DbContext.Accounts.SingleAsync(a => a.Email == TestEmail);
        Assert.True(BCrypt.Net.BCrypt.Verify("NewPassword1!", account.PasswordHash));
    }

    [Fact]
    public async Task ResetPassword_WrongCode_Returns400()
    {
        await SeedAccount(TestEmail, state: EAccountState.Active);
        SetupRedisStringGet($"ResetCode_{TestEmail}", "000000");

        var controller = CreateController();
        var result = await controller.ResetPassword(new ResetPasswordRequest
        {
            Email = TestEmail, VerifyCode = ResetCode, ResetPassword = "NewPassword1!"
        });

        Assert.Equal(400, GetStatusCode(result));
    }

    [Fact]
    public async Task ResetPassword_AccountNotFound_Returns404()
    {
        SetupRedisStringGet($"ResetCode_nobody@example.com", ResetCode);

        var controller = CreateController();
        var result = await controller.ResetPassword(new ResetPasswordRequest
        {
            Email = "nobody@example.com", VerifyCode = ResetCode, ResetPassword = "NewPassword1!"
        });

        Assert.Equal(404, GetStatusCode(result));
    }

    [Fact]
    public async Task ResetPassword_CleansUpRedis()
    {
        await SeedAccount(TestEmail, state: EAccountState.Active);
        SetupRedisStringGet($"ResetCode_{TestEmail}", ResetCode);

        var controller = CreateController();
        await controller.ResetPassword(new ResetPasswordRequest
        {
            Email = TestEmail, VerifyCode = ResetCode, ResetPassword = "NewPassword1!"
        });

        MockRedisDb.Verify(d => d.KeyDeleteAsync(new RedisKey($"ResetCode_{TestEmail}"), It.IsAny<CommandFlags>()), Times.Once);
        MockRedisDb.Verify(d => d.KeyDeleteAsync(new RedisKey($"ResetCooldown_{TestEmail}"), It.IsAny<CommandFlags>()), Times.Once);
    }

    #endregion
}
