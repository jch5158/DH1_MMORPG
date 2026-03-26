using DbMigration.Data.Table;
using Microsoft.EntityFrameworkCore;

namespace DbMigration.Data
{
    public class GameDbContext(DbContextOptions<GameDbContext> options) : DbContext(options)
    {
        public DbSet<PlayerCharacter> PlayerCharacters { get; set; }

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            base.OnModelCreating(modelBuilder);

            modelBuilder.Entity<PlayerCharacter>()
                .Property(pc => pc.CreatedAt)
                .HasDefaultValueSql("CURRENT_TIMESTAMP(6)");
        }
    }
}
