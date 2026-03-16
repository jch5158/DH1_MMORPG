namespace LoginServer.DTOs.Auth
{
    public class ResetPasswordRequest
    {
        public required string Email { get; set; }
        public required string ResetCode { get; set; }
        public required string NewPassword { get; set; }
    }
}
