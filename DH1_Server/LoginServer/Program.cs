using System.Diagnostics;
using LoginServer.Data;
using Microsoft.EntityFrameworkCore;
using StackExchange.Redis;

var builder = WebApplication.CreateBuilder(args);

var connectionString = builder.Configuration.GetConnectionString("AccountDbConnection");
Debug.Assert(!string.IsNullOrEmpty(connectionString), "string.IsNullOrEmpty(connectionString)");
builder.Services.AddDbContext<AccountDbContext>(options =>
{
    options.UseMySql(connectionString, ServerVersion.AutoDetect(connectionString));
});

var redisConnectionString = builder.Configuration.GetConnectionString("RedisConnection");
Debug.Assert(!string.IsNullOrEmpty(redisConnectionString), "string.IsNullOrEmpty(redisConnectionString)");
builder.Services.AddSingleton<IConnectionMultiplexer>(ConnectionMultiplexer.Connect(redisConnectionString));

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
