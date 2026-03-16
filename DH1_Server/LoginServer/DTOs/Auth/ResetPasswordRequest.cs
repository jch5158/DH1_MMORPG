namespace LoginServer.DTOs.Auth
{
    public class ResetPasswordRequest
    {
        public required string Email { get; set; }
        public required string VerifyCode { get; set; }
        public required string ResetPassword { get; set; }
    }
}
