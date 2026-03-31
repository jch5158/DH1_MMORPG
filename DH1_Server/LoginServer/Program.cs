using LoginServer.Data;
using LoginServer.Services;
using Microsoft.EntityFrameworkCore;
using StackExchange.Redis;

var builder = WebApplication.CreateBuilder(args);

// 환경변수 치환: ${ENV_VAR} 패턴을 실제 환경변수 값으로 대체
string ResolveEnvVars(string value)
{
    return System.Text.RegularExpressions.Regex.Replace(value, @"\$\{(\w+)\}", match =>
    {
        var envVar = match.Groups[1].Value;
        return Environment.GetEnvironmentVariable(envVar) ?? match.Value;
    });
}

builder.Configuration["SmtpSettings:Host"] = ResolveEnvVars(
    builder.Configuration["SmtpSettings:Host"] ?? string.Empty);
builder.Configuration["SmtpSettings:SenderEmail"] = ResolveEnvVars(
    builder.Configuration["SmtpSettings:SenderEmail"] ?? string.Empty);
builder.Configuration["SmtpSettings:AppPassword"] = ResolveEnvVars(
    builder.Configuration["SmtpSettings:AppPassword"] ?? string.Empty);

var connectionString = ResolveEnvVars(
    builder.Configuration.GetConnectionString("AccountDbConnection")
    ?? throw new InvalidOperationException("appsettings.json에 AccountDbConnection 설정이 없습니다."));

var redisConnectionString = ResolveEnvVars(
    builder.Configuration.GetConnectionString("RedisConnection")
    ?? throw new InvalidOperationException("appsettings.json에 RedisConnection 설정이 없습니다."));

builder.Services.AddDbContext<AccountDbContext>(options =>
{
    options.UseMySql(connectionString, ServerVersion.AutoDetect(connectionString));
});

builder.Services.AddSingleton<IConnectionMultiplexer>(ConnectionMultiplexer.Connect(redisConnectionString));
builder.Services.AddSingleton<IEmailQueue, EmailQueue>();
builder.Services.AddHostedService<EmailBackgroundService>();
builder.Services.AddScoped<GatewaySelector>();
builder.Services.AddScoped<AuthService>();
builder.Services.AddControllers();
builder.Services.AddOpenApi();

var app = builder.Build();
if (app.Environment.IsDevelopment())
{
    app.MapOpenApi();
}

app.UseHttpsRedirection();
app.UseAuthorization();
app.MapGet("/health", () => Results.Ok(new { status = "ok" }));
app.MapControllers();

app.Run();
