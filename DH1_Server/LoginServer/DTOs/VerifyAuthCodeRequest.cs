using System.ComponentModel.DataAnnotations;

namespace LoginServer.DTOs
{
    public class VerifyAuthCodeRequest
    {
        [Required(ErrorMessage = "이메일을 입력해주세요.")]
        [EmailAddress(ErrorMessage = "유효한 이메일 형식이 아닙니다.")]
        public required string Email { get; set; }

        [Required(ErrorMessage = "인증 번호를 입력해주세요.")]
        [StringLength(6, ErrorMessage = "인증번호가 올바르지 않습니다.")]
        public required string AuthCode { get; set; }
    }
}
