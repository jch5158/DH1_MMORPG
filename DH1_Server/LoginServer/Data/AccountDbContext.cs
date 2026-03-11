using Microsoft.EntityFrameworkCore;

namespace LoginServer.Data
{
    public class AccountDbContext(DbContextOptions<AccountDbContext> options) : DbContext(options)
    {
        public DbSet<Account>? Accounts { get; set; }

        // DB 구조 및 제약조건을 상세하게 세팅하는 메서드
        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            base.OnModelCreating(modelBuilder);

            // 1. Email 컬럼 고유(Unique) 제약 조건 추가 (중복 가입 방지)
            modelBuilder.Entity<Account>()
                .HasIndex(account => account.Email)
                .IsUnique();

            // Account 엔티티의 CreatedAt 속성에 MySQL 내장 함수(CURRENT_TIMESTAMP)를 기본값으로 박아넣음
            modelBuilder.Entity<Account>()
                .Property(account => account.CreatedAt)
                .HasDefaultValueSql("CURRENT_TIMESTAMP(6)");
        }
    }
}
