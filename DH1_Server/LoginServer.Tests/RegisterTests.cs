using LoginServer.Data.Table;
using LoginServer.DTOs.Auth;
using LoginServer.Services;
using Microsoft.EntityFrameworkCore;
using Moq;
using StackExchange.Redis;

namespace LoginServer.Tests;

public class RegisterTests : AuthControllerTestBase
{
    private const string TestEmail = "register@example.com";

    public RegisterTests()
    {
        SetupRedisKeyExists($"Cooldown_{TestEmail}", false);
        SetupRedisStringSet();
    }

    [Fact]
    public async Task Register_NewEmail_CreatesAccountAndSendsEmail()
    {
        var controller = CreateController();
        var result = await controller.Register(new RegisterRequest { Email = TestEmail, Password = TestPassword });

        Assert.Equal(200, GetStatusCode(result));

        var account = await DbContext.Accounts.SingleOrDefaultAsync(a => a.Email == TestEmail);
        Assert.NotNull(account);
        Assert.Equal(EAccountState.EmailUnverified, account.AccountState);

        MockEmailQueue.Verify(e => e.QueueEmailAsync(It.Is<EmailJob>(j => j.ToEmail == TestEmail)), Times.Once);
    }

    [Fact]
    public async Task Register_ExistingActiveEmail_Returns400()
    {
        await SeedAccount(TestEmail, state: EAccountState.Active);

        var controller = CreateController();
        var result = await controller.Register(new RegisterRequest { Email = TestEmail, Password = TestPassword });

        Assert.Equal(400, GetStatusCode(result));
        Assert.Equal("REGISTERED_EMAIL", GetResponseProperty(result, "code"));
    }

    [Fact]
    public async Task Register_ExistingUnverifiedEmail_Returns403()
    {
        await SeedAccount(TestEmail, state: EAccountState.EmailUnverified);

        var controller = CreateController();
        var result = await controller.Register(new RegisterRequest { Email = TestEmail, Password = TestPassword });

        Assert.Equal(403, GetStatusCode(result));
        Assert.Equal("EMAIL_UNVERIFIED", GetResponseProperty(result, "code"));
    }

    [Fact]
    public async Task Register_PasswordIsHashed()
    {
        var controller = CreateController();
        await controller.Register(new RegisterRequest { Email = TestEmail, Password = TestPassword });

        var account = await DbContext.Accounts.SingleAsync(a => a.Email == TestEmail);
        Assert.NotEqual(TestPassword, account.PasswordHash);
        Assert.True(BCrypt.Net.BCrypt.Verify(TestPassword, account.PasswordHash));
    }
}
