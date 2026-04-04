using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using Microsoft.EntityFrameworkCore;

namespace LoginServer.Data.Table
{
    [Table("account_unregistered")]
    [Index(nameof(Email), IsUnique = true)]
    public class AccountUnregistered
    {
        [Key]
        [DatabaseGenerated(DatabaseGeneratedOption.None)]
        [Column("account_id")]
        public long AccountId { get; set; }

        [StringLength(254)]
        [Column("email")]
        public required string Email { get; set; } // 복구를 위해 이메일 해시나 암호화된 형태로 보관 권장

        [Column("requested_at")]
        public required DateTime RequestedAt { get; set; }

        // 실제 물리적 삭제 또는 완전 익명화가 예정된 날짜
        [Column("scheduled_delete_at")]
        public required DateTime ScheduledDeleteAt { get; set; }
    }
}
