using LoginServer.Data;
using LoginServer.Services;
using Microsoft.EntityFrameworkCore;
using StackExchange.Redis;

var builder = WebApplication.CreateBuilder(args);

// Fail-Fast: 연결 문자열이 없으면 서버 구동을 즉시 중단하고 명확한 에러를 띄움
var connectionString = builder.Configuration.GetConnectionString("AccountDbConnection")
                       ?? throw new InvalidOperationException("appsettings.json에 AccountDbConnection 설정이 없습니다.");
var redisConnectionString = builder.Configuration.GetConnectionString("RedisConnection")
                            ?? throw new InvalidOperationException("appsettings.json에 RedisConnection 설정이 없습니다.");

builder.Services.AddDbContext<AccountDbContext>(options =>
{
    options.UseMySql(connectionString, ServerVersion.AutoDetect(connectionString));
});

builder.Services.AddSingleton<IConnectionMultiplexer>(ConnectionMultiplexer.Connect(redisConnectionString));
builder.Services.AddSingleton<IEmailQueue, EmailQueue>();
builder.Services.AddHostedService<EmailBackgroundService>();
builder.Services.AddControllers();
builder.Services.AddOpenApi();

var app = builder.Build();
if (app.Environment.IsDevelopment())
{
    app.MapOpenApi();
}

app.UseHttpsRedirection();
app.UseAuthorization();
app.MapControllers();

app.Run();