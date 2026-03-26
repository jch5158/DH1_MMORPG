using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace DbMigration.Data.Table
{
    [Table("account_sanction")]
    [Index(nameof(AccountId), nameof(StartDate), nameof(EndDate))]
    public class AccountSanction
    {
        [Key]
        [DatabaseGenerated(DatabaseGeneratedOption.Identity)]
        [Column("sanction_id")]
        public long SanctionId { get; set; }

        [Column("account_id")]
        public long AccountId { get; set; }

        [Column("start_date")]
        public required DateTime StartDate { get; set; }

        [Column("end_date")]
        public DateTime? EndDate { get; set; }

        [StringLength(255)]
        [Column("reason")]
        public required string Reason { get; set; }

        [ForeignKey("AccountId")]
        public Account Account { get; set; } = null!;
    }
}
