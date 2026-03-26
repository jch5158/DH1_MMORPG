using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace DbMigration.Data.Table
{
    [Table("player_character")]
    [Index(nameof(AccountId))]
    [Index(nameof(CharacterName), IsUnique = true)]
    public class PlayerCharacter
    {
        [Key]
        [DatabaseGenerated(DatabaseGeneratedOption.Identity)]
        [Column("character_id")]
        public long CharacterId { get; set; }

        [Column("account_id")]
        public long AccountId { get; set; }

        [StringLength(50)]
        [Column("character_name")]
        public required string CharacterName { get; set; }

        [Column("level")]
        public int Level { get; set; } = 1;

        [Column("experience")]
        public long Experience { get; set; } = 0;

        [Column("position_x")]
        public float PositionX { get; set; } = 0;

        [Column("position_y")]
        public float PositionY { get; set; } = 0;

        [Column("position_z")]
        public float PositionZ { get; set; } = 0;

        [Column("rotation_yaw")]
        public float RotationYaw { get; set; } = 0;

        [Column("world_id")]
        public int WorldId { get; set; } = 1;

        [Column("created_at")]
        public DateTime? CreatedAt { get; set; }

        [Column("last_played_at")]
        public DateTime? LastPlayedAt { get; set; }
    }
}
