using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace LoginServer.Data.Table
{
    [Table("account_sanction")]
    [Index(nameof(AccountId), nameof(StartDate), nameof(EndDate))]
    public class AccountSanction
    {
        [Key]
        [DatabaseGenerated(DatabaseGeneratedOption.Identity)] // DB에 INSERT 시 자동으로 1씩 증가
        [Column("sanction_id")]
        public long SanctionId { get; set; }

        [Column("account_id")]
        public long AccountId { get; set; }

        // 정지 시작일
        [Column("start_date")]
        public required DateTime StartDate { get; set; }

        // 정지 종료일 (Null이면 영구 정지)
        [Column("end_date")]
        public DateTime? EndDate { get; set; }

        // 제재 사유
        [StringLength(255)]
        [Column("reason")]
        public required string Reason { get; set; }

        // 외래키 설정
        [ForeignKey("AccountId")]
        public Account Account { get; set; } = null!;
    }
}
