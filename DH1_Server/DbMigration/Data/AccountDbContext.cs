using DbMigration.Data.Table;
using Microsoft.EntityFrameworkCore;

namespace DbMigration.Data
{
    public class AccountDbContext(DbContextOptions<AccountDbContext> options) : DbContext(options)
    {
        public DbSet<Account> Accounts { get; set; }
        public DbSet<AccountSanction> AccountSanctions { get; set; }
        public DbSet<AccountUnregistered> AccountUnregistereds { get; set; }

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            base.OnModelCreating(modelBuilder);

            modelBuilder.Entity<Account>()
                .HasIndex(account => account.Email)
                .IsUnique();

            modelBuilder.Entity<Account>()
                .Property(account => account.CreatedAt)
                .HasDefaultValueSql("CURRENT_TIMESTAMP(6)");
        }
    }
}
