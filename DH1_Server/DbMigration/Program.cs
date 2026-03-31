using DbMigration.Data;
using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Design;

// EF Core 마이그레이션 CLI 도구가 사용하는 Design-Time 팩토리
// dotnet ef migrations add <Name> --context <ContextName> --output-dir Migrations/<Folder>
// dotnet ef database update --context <ContextName>
//
// dotnet ef 는 .env 를 자동으로 읽지 않습니다. 아래 LoadDotEnvFromAncestors() 가
// 현재 디렉터리에서 위로 올라가며 첫 번째 .env 를 찾아 DH1_MYSQL_* 를 채웁니다.
// 이미 설정된 환경 변수는 덮어쓰지 않습니다.

namespace DbMigration
{
    internal static class DesignTimeMySqlEnv
    {
        private static int s_loaded;

        public static void EnsureLoaded()
        {
            if (Interlocked.Exchange(ref s_loaded, 1) != 0)
            {
                return;
            }

            TryLoadDotEnvFromAncestors(Directory.GetCurrentDirectory());
        }

        private static void TryLoadDotEnvFromAncestors(string? startDir)
        {
            if (string.IsNullOrEmpty(startDir))
            {
                return;
            }

            try
            {
                for (var dir = new DirectoryInfo(startDir); dir != null; dir = dir.Parent)
                {
                    var path = Path.Combine(dir.FullName, ".env");
                    if (!File.Exists(path))
                    {
                        continue;
                    }

                    foreach (var raw in File.ReadAllLines(path))
                    {
                        var line = raw.Trim();
                        if (line.Length == 0 || line.StartsWith("#", StringComparison.Ordinal))
                        {
                            continue;
                        }

                        var eq = line.IndexOf('=');
                        if (eq <= 0)
                        {
                            continue;
                        }

                        var key = line[..eq].Trim();
                        var value = line[(eq + 1)..].Trim();
                        if (value.Length >= 2 &&
                            ((value[0] == '"' && value[^1] == '"') ||
                             (value[0] == '\'' && value[^1] == '\'')))
                        {
                            value = value[1..^1];
                        }

                        if (Environment.GetEnvironmentVariable(key) is null)
                        {
                            Environment.SetEnvironmentVariable(key, value);
                        }
                    }

                    return;
                }
            }
            catch
            {
                // design-time: ignore .env read errors; caller will fail with a clear DB message
            }
        }

        public static string BuildConnectionString(string databaseName)
        {
            EnsureLoaded();
            var host = Environment.GetEnvironmentVariable("DH1_MYSQL_HOST") ?? "127.0.0.1";
            var port = Environment.GetEnvironmentVariable("DH1_MYSQL_PORT") ?? "3306";
            var user = Environment.GetEnvironmentVariable("DH1_MYSQL_USER") ?? "root";
            var password = Environment.GetEnvironmentVariable("DH1_MYSQL_PASSWORD") ?? "";
            return $"Server={host};Port={port};Database={databaseName};User={user};Password={password};";
        }
    }

    public class AccountDbContextFactory : IDesignTimeDbContextFactory<AccountDbContext>
    {
        public AccountDbContext CreateDbContext(string[] args)
        {
            var connectionString = DesignTimeMySqlEnv.BuildConnectionString("dh1_account_db");

            var optionsBuilder = new DbContextOptionsBuilder<AccountDbContext>();
            optionsBuilder.UseMySql(connectionString, ServerVersion.AutoDetect(connectionString));

            return new AccountDbContext(optionsBuilder.Options);
        }
    }

    public class GameDbContextFactory : IDesignTimeDbContextFactory<GameDbContext>
    {
        public GameDbContext CreateDbContext(string[] args)
        {
            var connectionString = DesignTimeMySqlEnv.BuildConnectionString("dh1_game_db");

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
