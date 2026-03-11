using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace LoginServer.Data
{
    // 1. 계정 권한 Enum 정의 (보통 파일 상단이나 별도 파일로 분리)
    public enum EAccountRole
    {
        User = 0,
        Admin = 1,
        GameMaster = 2 // 필요에 따라 추가 가능
    }

    [Table("account")]
    public class Account
    {
        [Key]
        [Column("account_id")]
        public int AccountId { get; set; }

        [EmailAddress]
        [StringLength(254)]
        [Column("email")]
        public required string Email { get; set; }

        [StringLength(128)]
        [Column("password_hash")]
        public required string PasswordHash { get; set; }

        // 2. Enum 타입의 속성 추가 (기본값을 일반 유저로 설정)
        [Column("account_role")]
        public EAccountRole Role { get; set; } = EAccountRole.User;

        [Column("last_login")]
        public DateTime? LastLogin { get; set; }

        [Column("created_at")]
        public DateTime? CreatedAt { get; set; }

        [Column("is_email_verified")]
        public bool IsEmailVerified { get; set; } = false;

        [Column("is_banned")]
        public bool IsBanned { get; set; } = false;
    }
}
