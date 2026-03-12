using System.ComponentModel.DataAnnotations;

namespace LoginServer.DTOs.Auth
{
    public class LoginRequest
    {
        [Required(ErrorMessage = "이메일을 입력해주세요.")]
        [EmailAddress(ErrorMessage = "유효한 이메일 형식이 아닙니다.")]
        public required string Email { get; set; }

        [Required(ErrorMessage = "비밀번호를 입력해주세요.")]
        [StringLength(128, MinimumLength = 8, ErrorMessage = "비밀번호는 최소 8자 이상이어야 합니다.")]
        public required string Password { get; set; }
    }
}
