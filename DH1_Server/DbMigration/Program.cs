using DbMigration.Data;
using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Design;

// EF Core 마이그레이션 CLI 도구가 사용하는 Design-Time 팩토리
// dotnet ef migrations add <Name> --context <ContextName> --output-dir Migrations/<Folder>
// dotnet ef database update --context <ContextName>

namespace DbMigration
{
    public class AccountDbContextFactory : IDesignTimeDbContextFactory<AccountDbContext>
    {
        public AccountDbContext CreateDbContext(string[] args)
        {
            var host = Environment.GetEnvironmentVariable("DH1_MYSQL_HOST") ?? "127.0.0.1";
            var port = Environment.GetEnvironmentVariable("DH1_MYSQL_PORT") ?? "3306";
            var password = Environment.GetEnvironmentVariable("DH1_MYSQL_PASSWORD") ?? "";

            var connectionString = $"Server={host};Port={port};Database=dh1_account_db;User=root;Password={password};";

            var optionsBuilder = new DbContextOptionsBuilder<AccountDbContext>();
            optionsBuilder.UseMySql(connectionString, ServerVersion.AutoDetect(connectionString));

            return new AccountDbContext(optionsBuilder.Options);
        }
    }

    public class GameDbContextFactory : IDesignTimeDbContextFactory<GameDbContext>
    {
        public GameDbContext CreateDbContext(string[] args)
        {
            var host = Environment.GetEnvironmentVariable("DH1_MYSQL_HOST") ?? "127.0.0.1";
            var port = Environment.GetEnvironmentVariable("DH1_MYSQL_PORT") ?? "3306";
            var password = Environment.GetEnvironmentVariable("DH1_MYSQL_PASSWORD") ?? "";

            var connectionString = $"Server={host};Port={port};Database=dh1_game_db;User=root;Password={password};";

            var optionsBuilder = new DbContextOptionsBuilder<GameDbContext>();
            optionsBuilder.UseMySql(connectionString, ServerVersion.AutoDetect(connectionString));

            return new GameDbContext(optionsBuilder.Options);
        }
    }

    public class Program
    {
        public static void Main(string[] args)
        {
            Console.WriteLine("DH1 Database Migration Tool");
            Console.WriteLine("Usage:");
            Console.WriteLine("  dotnet ef migrations add <Name> --context AccountDbContext --output-dir Migrations/Account");
            Console.WriteLine("  dotnet ef migrations add <Name> --context GameDbContext --output-dir Migrations/Game");
            Console.WriteLine("  dotnet ef database update --context AccountDbContext");
            Console.WriteLine("  dotnet ef database update --context GameDbContext");
        }
    }
}
