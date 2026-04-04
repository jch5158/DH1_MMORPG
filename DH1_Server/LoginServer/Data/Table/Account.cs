using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace LoginServer.Data.Table
{
    // 1. 계정 권한 Enum 정의 (보통 파일 상단이나 별도 파일로 분리)
    public enum EAccountRole : byte
    {
        User = 0,
        Admin = 1,
        GameMaster = 2 // 필요에 따라 추가 가능
    }

    public enum EAccountState : byte
    {
        EmailUnverified = 0,    // 이메일 인증 필요
        Active = 1,             // 정상 이용 가능 (인증 완료)
        Suspended = 2,          // 임시 정지
        Banned = 3,             // 영구 정지
        PendingUnregister = 4   // 탈퇴 유예 기간
    }

    [Table("account")]
    [Index(nameof(Email), IsUnique = true)]
    public class Account
    {
        [Key]
        [DatabaseGenerated(DatabaseGeneratedOption.Identity)] // DB에 INSERT 시 자동으로 1씩 증가
        [Column("account_id")]
        public long AccountId { get; set; }

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

        [Column("account_state")]
        public EAccountState AccountState { get; set; } = EAccountState.EmailUnverified;
    }
}
