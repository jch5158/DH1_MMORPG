using LoginServer.Data;
using Microsoft.AspNetCore.Identity.Data;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using StackExchange.Redis;
using System.Diagnostics;
using System.Security.Cryptography;

namespace LoginServer.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class AuthController(AccountDbContext dbContext, IConnectionMultiplexer redisConnection) : ControllerBase
    {
        private AccountDbContext DbContext { get; } = dbContext;
        private IConnectionMultiplexer RedisConnection { get; } = redisConnection;

        [HttpPost("Login")]
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
    }
}
