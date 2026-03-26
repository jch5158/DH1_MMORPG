using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using Microsoft.EntityFrameworkCore;

namespace DbMigration.Data.Table
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
        public required string Email { get; set; }

        [Column("requested_at")]
        public required DateTime RequestedAt { get; set; }

        [Column("scheduled_delete_at")]
        public required DateTime ScheduledDeleteAt { get; set; }
    }
}
