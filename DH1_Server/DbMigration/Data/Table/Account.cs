using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace DbMigration.Data.Table
{
    public enum EAccountRole : byte
    {
        User = 0,
        Admin = 1,
        GameMaster = 2
    }

    public enum EAccountState : byte
    {
        EmailUnverified = 0,
        Active = 1,
        Suspended = 2,
        Banned = 3,
        PendingUnregister = 4
    }

    [Table("account")]
    [Index(nameof(Email), IsUnique = true)]
    public class Account
    {
        [Key]
        [DatabaseGenerated(DatabaseGeneratedOption.Identity)]
        [Column("account_id")]
        public long AccountId { get; set; }

        [EmailAddress]
        [StringLength(254)]
        [Column("email")]
        public required string Email { get; set; }

        [StringLength(128)]
        [Column("password_hash")]
        public required string PasswordHash { get; set; }

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
