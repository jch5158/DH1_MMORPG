using System.ComponentModel.DataAnnotations;

namespace LoginServer.DTOs.Account
{
    public class AuthCodeRequest
    {
        [Required(ErrorMessage = "이메일을 입력해주세요.")]
        [EmailAddress(ErrorMessage = "유효한 이메일 형식이 아닙니다.")]
        public required string Email { get; set; }
    }
}
