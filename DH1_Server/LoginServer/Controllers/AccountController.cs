using LoginServer.Data;
using Microsoft.AspNetCore.Identity.Data;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

namespace LoginServer.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class AccountController(AccountDbContext dbContext) : ControllerBase
    {
        [HttpPost("register")]
        public async Task<IActionResult> Register([FromBody] RegisterRequest request)
        {
            var emailExists = await dbContext.Accounts.AnyAsync(a => a.Email == request.Email);
            if (emailExists)
            {
                return Conflict(new { message = "이미 사용 중인 이메일입니다." });
            }

            var newAccount = new Account
            {
                Email = request.Email,
                PasswordHash = BCrypt.Net.BCrypt.HashPassword(request.Password)
            };

            dbContext.Accounts.Add(newAccount);
            await dbContext.SaveChangesAsync();

            return Ok(new { message = "회원가입이 성공적으로 완료되었습니다." });
        }
    }
}
