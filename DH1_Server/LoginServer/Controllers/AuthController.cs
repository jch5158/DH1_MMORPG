using LoginServer.Data;
using LoginServer.Data.Table;
using Microsoft.AspNetCore.Identity.Data;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using MimeKit;
using StackExchange.Redis;
using System.Diagnostics;
using System.Security.Cryptography;
using MailKit.Net.Smtp;
using LoginServer.DTOs.Auth;
using LoginRequest = LoginServer.DTOs.Auth.LoginRequest;
using RegisterRequest = LoginServer.DTOs.Auth.RegisterRequest;

namespace LoginServer.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class AuthController(AccountDbContext dbContext, IConnectionMultiplexer redisConnection, IConfiguration smtpConfig) : ControllerBase
    {
        private AccountDbContext DbContext { get; } = dbContext;
        private IConnectionMultiplexer RedisConnection { get; } = redisConnection;

        private IConfiguration SmtpConfig { get; } = smtpConfig;

        private async Task<bool> SendVerifyCodeAsync(string email)
        {
            var authCode = Random.Shared.Next(100000, 999999).ToString();

            var redisDb = RedisConnection.GetDatabase();
            var redisKey = $"VerifyCode_{email}";
            var isSetAuthCode = await redisDb.StringSetAsync(redisKey, authCode, TimeSpan.FromMinutes(3));
            if (!isSetAuthCode)
            {
                return false;
            }

            var message = new MimeMessage();
            message.From.Add(new MailboxAddress(SmtpConfig["SmtpSettings:SenderName"], SmtpConfig["SmtpSettings:SenderEmail"] ?? string.Empty));
            message.To.Add(new MailboxAddress("", email));
            message.Subject = "[DH1_MMORPG] 회원가입 인증번호 안내";
            message.Body = new TextPart("plain")
            {
                Text = $"안녕하세요.\n회원가입 인증번호는 [{authCode}] 입니다.\n3분 이내에 입력해주세요."
            };

            using var client = new SmtpClient();
            await client.ConnectAsync(SmtpConfig["SmtpSettings:Host"], int.Parse(SmtpConfig["SmtpSettings:Port"]!), MailKit.Security.SecureSocketOptions.StartTls);
            await client.AuthenticateAsync(SmtpConfig["SmtpSettings:SenderEmail"], SmtpConfig["SmtpSettings:AppPassword"]);
            await client.SendAsync(message);
            await client.DisconnectAsync(true);

            return true;
        }


        [HttpPost("login")]
        public async Task<IActionResult> Login([FromBody] LoginRequest request)
        {
            Debug.Assert(DbContext.Accounts != null);

            var account =
                await DbContext.Accounts.SingleOrDefaultAsync(a => a.Email == request.Email);
            if (account == null || !BCrypt.Net.BCrypt.Verify(request.Password, account.PasswordHash))
            {
                return BadRequest(new { message = "회원 정보가 일치하지 않습니다." });
            }

            var tokenBytes = RandomNumberGenerator.GetBytes(32);
            var ticket = Convert.ToBase64String(tokenBytes);

            var redisDb = RedisConnection.GetDatabase();
            var expiry = TimeSpan.FromSeconds(30);

            var ticketKey = $"ticket:{ticket}";

            var isSet = await redisDb.StringSetAsync(ticketKey, account.AccountId.ToString(), expiry);
            if (!isSet)
            {
                return StatusCode(500, new { message = "접속 티켓 발급 중 오류가 발생했습니다." });
            }

            return Ok(new
            {
                message = "로그인 성공!",
                ticket,
                accountId = account.AccountId,
                gatewayIp = "192.168.1.100",
                gatewayPort = 9000
            });
        }

        [HttpPost("register")]
        public async Task<IActionResult> Register([FromBody] RegisterRequest request)
        {
            Debug.Assert(DbContext.Accounts != null);
            var existingAccount = await DbContext.Accounts.FirstOrDefaultAsync(account => account.Email == request.Email);
            if (existingAccount != null)
            {
                if (existingAccount.AccountState != EAccountState.EmailUnverified)
                {
                    return Conflict(new { message = "이미 사용중인 이메일입니다." });
                }

                existingAccount.PasswordHash = BCrypt.Net.BCrypt.HashPassword(request.Password);
                await DbContext.SaveChangesAsync();

                var redisDb = RedisConnection.GetDatabase();
                var redisKey = $"VerifyCode_{request.Email}";
                await redisDb.KeyDeleteAsync(redisKey);
            }
            else
            {
                var newAccount = new Account
                {
                    Email = request.Email,
                    PasswordHash = BCrypt.Net.BCrypt.HashPassword(request.Password),
                };

                DbContext.Accounts.Add(newAccount);
                await DbContext.SaveChangesAsync();
            }

            await SendVerifyCodeAsync(request.Email);

            return Ok(new { message = "회원가입이 완료되었습니다. 이메일 인증 부탁드립니다." });
        }

        [HttpPost("send-verify-code")]
        public async Task<IActionResult> SendVerifyCode([FromBody] SendVerifyCodeRequest request)
        {
            Debug.Assert(DbContext.Accounts != null);
            var isExistingEmail = await DbContext.Accounts.AnyAsync(account => account.Email == request.Email);
            if (!isExistingEmail)
            {
                return Conflict(new { message = "인증 코드 발송에 실패했습니다." });
            }

            await SendVerifyCodeAsync(request.Email);

            return Ok(new { message = "인증번호가 발송되었습니다." });
        }

        [HttpPost("verify-code")]
        public async Task<IActionResult> CompareVerifyAuthCode([FromBody] VerifyCodeRequest request)
        {
            var redisDb = RedisConnection.GetDatabase();
            var redisKey = $"VerifyCode_{request.Email}";

            var savedCode = await redisDb.StringGetAsync(redisKey);
            if (!savedCode.HasValue || savedCode.ToString() != request.VerifyCode)
            {
                return BadRequest(new { message = "인증번호가 일치하지 않습니다." });
            }

            Debug.Assert(DbContext.Accounts != null);
            var account = await DbContext.Accounts.SingleOrDefaultAsync(a => a.Email == request.Email && a.AccountState == EAccountState.EmailUnverified);
            if (account != null)
            {
                account.AccountState = EAccountState.Active;
                await DbContext.SaveChangesAsync();
            }
            else
            {
                return BadRequest(new { message = "이메일 인증에 실패하였습니다." });
            }

            await redisDb.KeyDeleteAsync(redisKey);

            return Ok(new { message = "이메일 인증이 완료되었습니다." });
        }
    }
}
