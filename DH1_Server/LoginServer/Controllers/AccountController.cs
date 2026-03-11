using LoginServer.Data;
using LoginServer.DTOs;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using StackExchange.Redis;
using System.Diagnostics;
using MailKit.Net.Smtp;
using MimeKit;

namespace LoginServer.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class AccountController(AccountDbContext dbContext, IConnectionMultiplexer redisConnection, IConfiguration smtpConfig) : ControllerBase
    {
        private AccountDbContext DbContext { get; } = dbContext;
        private IConnectionMultiplexer RedisConnection { get; } = redisConnection;

        private IConfiguration SmtpConfig { get; } = smtpConfig;

        [HttpPost("register")]
        public async Task<IActionResult> Register([FromBody] RegisterRequest request)
        {
            Debug.Assert(DbContext.Accounts != null, "DbContext.Accounts != null");
            var emailExists = await DbContext.Accounts.AnyAsync(a => a.Email == request.Email);
            if (emailExists)
            {
                return Conflict(new { message = "이미 사용 중인 이메일입니다." });
            }

            var newAccount = new Account
            {
                Email = request.Email,
                PasswordHash = BCrypt.Net.BCrypt.HashPassword(request.Password)
            };

            DbContext.Accounts.Add(newAccount);
            await DbContext.SaveChangesAsync();

            return Ok(new { message = "회원가입이 완료되었습니다. 이메일 인증 부탁드립니다." });
        }

        [HttpPost("auth-code")]
        public async Task<IActionResult> AuthCode([FromBody] AuthCodeRequest request)
        {
            Debug.Assert(DbContext.Accounts != null, "DbContext.Accounts != null");
            var emailExists = await DbContext.Accounts.AnyAsync(a => a.Email == request.Email && a.IsEmailVerified == false);
            if (!emailExists)
            {
                return Conflict(new { message = "인증 코드 발송에 실패했습니다." });
            }

            var authCode = Random.Shared.Next(100000, 999999).ToString();

            var redisDb = RedisConnection.GetDatabase();
            var redisKey = $"AuthCode_{request.Email}";
            await redisDb.StringSetAsync(redisKey, authCode, TimeSpan.FromMinutes(3));

            var message = new MimeMessage();
            message.From.Add(new MailboxAddress(SmtpConfig["SmtpSettings:SenderName"], SmtpConfig["SmtpSettings:SenderEmail"] ?? string.Empty));
            message.To.Add(new MailboxAddress("", request.Email));
            message.Subject = "[DH1_MMORPG] 회원가입 인증번호 안내";
            message.Body = new TextPart("plain")
            {
                Text = $"안녕하세요.\n회원가입 인증번호는 [{authCode}] 입니다.\n3분 이내에 입력해주세요."
            };

            using (var client = new SmtpClient())
            {
                // 포트 587과 TLS 보안 연결 사용
                await client.ConnectAsync(SmtpConfig["SmtpSettings:Host"], int.Parse(SmtpConfig["SmtpSettings:Port"]!), MailKit.Security.SecureSocketOptions.StartTls);
                await client.AuthenticateAsync(SmtpConfig["SmtpSettings:SenderEmail"], SmtpConfig["SmtpSettings:AppPassword"]);
                await client.SendAsync(message);
                await client.DisconnectAsync(true);
            }

            return Ok(new { message = "인증번호가 발송되었습니다." });
        }

        [HttpPost("verify-auth-code")]
        public async Task<IActionResult> VerifyAuthCode([FromBody] VerifyAuthCodeRequest request)
        {
            var redisDb = RedisConnection.GetDatabase(); // 네이밍에 따라 RedisConnection.GetDatabase()
            var redisKey = $"AuthCode_{request.Email}";

            var savedCode = await redisDb.StringGetAsync(redisKey);
            if (!savedCode.HasValue || savedCode.ToString() != request.AuthCode)
            {
                return BadRequest(new { message = "인증번호가 일치하지 않습니다." });
            }

            Debug.Assert(DbContext.Accounts != null, "DbContext.Accounts != null");
            var account = await DbContext.Accounts.SingleOrDefaultAsync(a => a.Email == request.Email && a.IsEmailVerified == false);
            if (account != null)
            {
                account.IsEmailVerified = true;
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
