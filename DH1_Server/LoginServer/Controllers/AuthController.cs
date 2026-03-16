using LoginServer.Data;
using LoginServer.Data.Table;
using LoginServer.DTOs.Auth;
using MailKit.Net.Smtp;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using MimeKit;
using StackExchange.Redis;
using System.Security.Cryptography;
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

        private async Task<IActionResult?> SendVerifyCodeAsync(string email)
        {
            var redisDb = RedisConnection.GetDatabase();

            // 1. 30초 쿨타임 체크
            var cooldownKey = $"Cooldown_{email}";
            if (await redisDb.KeyExistsAsync(cooldownKey))
            {
                return BadRequest(new { message = "30초 후에 다시 시도해주세요." });
            }

            var authCode = Random.Shared.Next(100000, 1000000).ToString();
            var redisKey = $"VerifyCode_{email}";

            await redisDb.StringSetAsync(redisKey, authCode, TimeSpan.FromMinutes(3));
            await redisDb.StringSetAsync(cooldownKey, "1", TimeSpan.FromSeconds(30));

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

            return null; // 성공 시 null 반환
        }


        [HttpPost("login")]
        public async Task<IActionResult> Login([FromBody] LoginRequest request)
        {
            // 성능 개선: 단순 읽기에는 AsNoTracking() 추가
            var account = await DbContext.Accounts.AsNoTracking()
                .SingleOrDefaultAsync(a => a.Email == request.Email);

            if (account == null || !BCrypt.Net.BCrypt.Verify(request.Password, account.PasswordHash))
            {
                return BadRequest(new { message = "회원 정보가 일치하지 않습니다." });
            }

            // 보안 결함 1 수정: 이메일 미인증 유저 로그인 차단
            if (account.AccountState == EAccountState.EmailUnverified)
            {
                return StatusCode(StatusCodes.Status403Forbidden, new
                {
                    code = "EMAIL_UNVERIFIED",
                    email = account.Email,
                    message = "이메일 인증이 완료되지 않은 계정입니다. 인증 페이지로 이동합니다."
                });
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
            var existingAccount = await DbContext.Accounts.AsNoTracking().FirstOrDefaultAsync(account => account.Email == request.Email);
            if (existingAccount != null)
            {
                if (existingAccount.AccountState == EAccountState.EmailUnverified)
                {
                    return StatusCode(StatusCodes.Status403Forbidden, new
                    {
                        code = "EMAIL_UNVERIFIED",
                        email = existingAccount.Email,
                        message = "이메일 인증이 완료되지 않은 계정입니다. 인증 페이지로 이동합니다."
                    });
                }

                return BadRequest(new { message = "이미 사용중인 이메일입니다." });
            }

            var newAccount = new Account
            {
                Email = request.Email,
                PasswordHash = BCrypt.Net.BCrypt.HashPassword(request.Password),
            };

            DbContext.Accounts.Add(newAccount);
            await DbContext.SaveChangesAsync();

            var sendResult = await SendVerifyCodeAsync(request.Email);
            return sendResult ?? Ok(new { message = "회원가입이 완료되었습니다. 이메일 인증 부탁드립니다." });
        }

        [HttpPost("send-verify-code")]
        public async Task<IActionResult> SendVerifyCode([FromBody] SendVerifyCodeRequest request)
        {
            // 보안 결함 2 수정: 이미 인증된 유저에게 스팸 메일 발송 차단
            var account = await DbContext.Accounts.AsNoTracking()
                .SingleOrDefaultAsync(a => a.Email == request.Email);
            if (account == null)
            {
                return NotFound(new { message = "가입된 이메일이 없습니다." });
            }

            if (account.AccountState != EAccountState.EmailUnverified)
            {
                return Conflict(new { message = "이미 인증이 완료된 계정입니다." });
            }

            var sendResult = await SendVerifyCodeAsync(request.Email);
            return sendResult ?? Ok(new { message = "인증번호가 발송되었습니다." });
        }

        [HttpPost("verify-code")]
        public async Task<IActionResult> CompareVerifyAuthCode([FromBody] VerifyCodeRequest request)
        {
            var redisDb = RedisConnection.GetDatabase();
            var redisKey = $"VerifyCode_{request.Email}";

            var savedCode = await redisDb.StringGetAsync(redisKey);
            if (!savedCode.HasValue || savedCode.ToString() != request.VerifyCode)
            {
                return BadRequest(new { message = "인증번호가 일치하지 않거나 만료되었습니다." });
            }

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
            await redisDb.KeyDeleteAsync($"Cooldown_{request.Email}");

            return Ok(new { message = "이메일 인증이 완료되었습니다." });
        }
    }
}
