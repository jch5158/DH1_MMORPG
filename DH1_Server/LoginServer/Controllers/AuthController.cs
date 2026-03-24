using LoginServer.DTOs.Auth;
using LoginServer.Services;
using Microsoft.AspNetCore.Mvc;

namespace LoginServer.Controllers
{
    [ApiController]
    [Route("api/[controller]")]
    public class AuthController(AuthService authService) : ControllerBase
    {
        [HttpPost("login")]
        public async Task<IActionResult> Login([FromBody] LoginRequest request)
        {
            var result = await authService.LoginAsync(request.Email, request.Password);

            if (!result.Success)
            {
                return StatusCode(result.StatusCode, new { code = result.ErrorCode, email = result.Email, message = result.ErrorMessage });
            }

            return Ok(new
            {
                ticket = result.Ticket,
                accountId = result.AccountId,
                gatewayIp = result.GatewayIp,
                gatewayPort = result.GatewayPort
            });
        }

        [HttpPost("register")]
        public async Task<IActionResult> Register([FromBody] RegisterRequest request)
        {
            var result = await authService.RegisterAsync(request.Email, request.Password);

            if (!result.Success)
            {
                return StatusCode(result.StatusCode, new { code = result.ErrorCode, email = result.Email, message = result.ErrorMessage });
            }

            return Ok(new { email = result.Email, message = result.ErrorMessage });
        }

        [HttpPost("send-verify-code")]
        public async Task<IActionResult> SendVerifyCode([FromBody] SendVerifyCodeRequest request)
        {
            var result = await authService.SendVerifyCodeAsync(request.Email);

            if (!result.Success)
            {
                return StatusCode(result.StatusCode, new { message = result.ErrorMessage });
            }

            return Ok(new { message = result.ErrorMessage });
        }

        [HttpPost("verify-code")]
        public async Task<IActionResult> CompareVerifyAuthCode([FromBody] VerifyCodeRequest request)
        {
            var result = await authService.VerifyCodeAsync(request.Email, request.VerifyCode);

            if (!result.Success)
            {
                return StatusCode(result.StatusCode, new { message = result.ErrorMessage });
            }

            return Ok(new { email = result.Email, message = result.ErrorMessage });
        }

        [HttpPost("forgot-password")]
        public async Task<IActionResult> ForgotPassword([FromBody] ForgotPasswordRequest request)
        {
            var result = await authService.ForgotPasswordAsync(request.Email);

            if (!result.Success)
            {
                return StatusCode(result.StatusCode, new { message = result.ErrorMessage });
            }

            return Ok(new { message = result.ErrorMessage });
        }

        [HttpPost("reset-password")]
        public async Task<IActionResult> ResetPassword([FromBody] ResetPasswordRequest request)
        {
            var result = await authService.ResetPasswordAsync(request.Email, request.VerifyCode, request.ResetPassword);

            if (!result.Success)
            {
                return StatusCode(result.StatusCode, new { message = result.ErrorMessage });
            }

            return Ok(new { email = result.Email, message = result.ErrorMessage });
        }
    }
}
