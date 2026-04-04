using System.ComponentModel.DataAnnotations;

namespace LoginServer.DTOs.Auth
{
    public class VerifyCodeRequest
    {
        [Required(ErrorMessage = "이메일을 입력해주세요.")]
        [EmailAddress(ErrorMessage = "유효한 이메일 형식이 아닙니다.")]
        public required string Email { get; set; }

        [Required(ErrorMessage = "인증 번호를 입력해주세요.")]
        [StringLength(6, MinimumLength = 6, ErrorMessage = "인증번호는 6자리여야 합니다.")]
        [RegularExpression(@"^\d{6}$", ErrorMessage = "인증번호는 6자리 숫자여야 합니다.")]
        public required string VerifyCode { get; set; }
    }
}
