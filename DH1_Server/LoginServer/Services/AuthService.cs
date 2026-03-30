using LoginServer.Data;
using LoginServer.Data.Table;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Configuration;
using StackExchange.Redis;
using System.Security.Cryptography;

namespace LoginServer.Services
{
    public class LoginResult
    {
        public bool Success { get; init; }
        public int StatusCode { get; init; } = 200;
        public string? ErrorCode { get; init; }
        public string? ErrorMessage { get; init; }
        public string? Email { get; init; }

        // 성공 시 데이터
        public string? Ticket { get; init; }
        public long AccountId { get; init; }
        public string? GatewayIp { get; init; }
        public int GatewayPort { get; init; }
    }

    public class RegisterResult
    {
        public bool Success { get; init; }
        public int StatusCode { get; init; } = 200;
        public string? ErrorCode { get; init; }
        public string? ErrorMessage { get; init; }
        public string? Email { get; init; }
    }

    public class VerifyResult
    {
        public bool Success { get; init; }
        public int StatusCode { get; init; } = 200;
        public string? ErrorMessage { get; init; }
        public string? Email { get; init; }
    }

    public class AuthService(
        AccountDbContext dbContext,
        IConnectionMultiplexer redisConnection,
        IEmailQueue emailQueue,
        GatewaySelector gatewaySelector,
        ILogger<AuthService> logger,
        IConfiguration? configuration = null)
    {
        private const int MaxLoginFailCount = 5;
        private const int LoginLockMinutes = 10;
        private const int TicketExpirySeconds = 30;
        private const int VerifyCodeExpiryMinutes = 3;
        private const int CooldownSeconds = 30;
        private const string EmailDeliveryConfigErrorMessage = "이메일 발송 설정이 올바르지 않습니다. 관리자에게 문의해주세요.";

        public async Task<LoginResult> LoginAsync(string email, string password)
        {
            var redisDb = redisConnection.GetDatabase();
            var lockKey = $"LoginLock_{email}";
            var failKey = $"LoginFail_{email}";

            if (await redisDb.KeyExistsAsync(lockKey))
            {
                return new LoginResult
                {
                    Success = false,
                    StatusCode = 429,
                    ErrorMessage = $"로그인 시도 횟수를 초과했습니다. {LoginLockMinutes}분 후 다시 시도해주세요."
                };
            }

            var account = await dbContext.Accounts.AsNoTracking()
                .SingleOrDefaultAsync(a => a.Email == email);

            if (account == null || !BCrypt.Net.BCrypt.Verify(password, account.PasswordHash))
            {
                var failCount = (int)await redisDb.StringIncrementAsync(failKey);
                await redisDb.KeyExpireAsync(failKey, TimeSpan.FromMinutes(LoginLockMinutes));

                if (failCount >= MaxLoginFailCount)
                {
                    await redisDb.StringSetAsync(lockKey, "1", TimeSpan.FromMinutes(LoginLockMinutes));
                    await redisDb.KeyDeleteAsync(failKey);
                    return new LoginResult
                    {
                        Success = false,
                        StatusCode = 429,
                        ErrorMessage = $"로그인 시도 횟수를 초과했습니다. {LoginLockMinutes}분 후 다시 시도해주세요."
                    };
                }

                return new LoginResult
                {
                    Success = false,
                    StatusCode = 400,
                    ErrorMessage = "회원 정보가 일치하지 않습니다."
                };
            }

            var accountStateResult = ValidateAccountState(account);
            if (accountStateResult != null)
            {
                return accountStateResult;
            }

            await redisDb.KeyDeleteAsync(failKey);

            var tokenBytes = RandomNumberGenerator.GetBytes(32);
            var ticket = Convert.ToBase64String(tokenBytes);
            var ticketKey = $"ticket:{ticket}";

            var isSet = await redisDb.StringSetAsync(ticketKey, account.AccountId.ToString(), TimeSpan.FromSeconds(TicketExpirySeconds));
            if (!isSet)
            {
                logger.LogError("Failed to set ticket in Redis for account {AccountId}", account.AccountId);
                return new LoginResult
                {
                    Success = false,
                    StatusCode = 500,
                    ErrorMessage = "접속 티켓 발급 중 오류가 발생했습니다."
                };
            }

            var bestGateway = await gatewaySelector.SelectBestGatewayAsync();
            if (bestGateway == null)
            {
                await redisDb.KeyDeleteAsync(ticketKey);
                return new LoginResult
                {
                    Success = false,
                    StatusCode = 503,
                    ErrorMessage = "접속 가능한 게이트웨이 서버가 없습니다."
                };
            }

            var (gatewayIp, gatewayPort) = bestGateway.Value;

            logger.LogInformation("Login successful for account {AccountId}, assigned gateway {Ip}:{Port}",
                account.AccountId, gatewayIp, gatewayPort);

            return new LoginResult
            {
                Success = true,
                Ticket = ticket,
                AccountId = account.AccountId,
                GatewayIp = gatewayIp,
                GatewayPort = gatewayPort
            };
        }

        public async Task<RegisterResult> RegisterAsync(string email, string password)
        {
            var existingAccount = await dbContext.Accounts.AsNoTracking()
                .FirstOrDefaultAsync(account => account.Email == email);

            if (existingAccount != null)
            {
                if (existingAccount.AccountState == EAccountState.EmailUnverified)
                {
                    return new RegisterResult
                    {
                        Success = false,
                        StatusCode = 403,
                        ErrorCode = "EMAIL_UNVERIFIED",
                        Email = existingAccount.Email,
                        ErrorMessage = "이메일 인증이 완료되지 않은 계정입니다. 인증 페이지로 이동합니다."
                    };
                }

                return new RegisterResult
                {
                    Success = false,
                    StatusCode = 400,
                    ErrorCode = "REGISTERED_EMAIL",
                    Email = existingAccount.Email,
                    ErrorMessage = "이미 사용중인 이메일입니다."
                };
            }

            var newAccount = new Account
            {
                Email = email,
                PasswordHash = BCrypt.Net.BCrypt.HashPassword(password),
            };

            dbContext.Accounts.Add(newAccount);
            await dbContext.SaveChangesAsync();

            var sendResult = await SendVerifyCodeInternalAsync(email);
            if (sendResult != null)
            {
                return new RegisterResult
                {
                    Success = false,
                    StatusCode = sendResult == EmailDeliveryConfigErrorMessage ? 503 : 400,
                    ErrorMessage = sendResult
                };
            }

            return new RegisterResult
            {
                Success = true,
                Email = email,
                ErrorMessage = "회원가입이 완료되었습니다. 이메일 인증 부탁드립니다."
            };
        }

        public async Task<VerifyResult> SendVerifyCodeAsync(string email)
        {
            var account = await dbContext.Accounts.AsNoTracking()
                .SingleOrDefaultAsync(a => a.Email == email);

            if (account == null)
            {
                return new VerifyResult { Success = false, StatusCode = 404, ErrorMessage = "가입된 이메일이 없습니다." };
            }

            if (account.AccountState != EAccountState.EmailUnverified)
            {
                return new VerifyResult { Success = false, StatusCode = 409, ErrorMessage = "이미 인증이 완료된 계정입니다." };
            }

            var sendError = await SendVerifyCodeInternalAsync(email);
            if (sendError != null)
            {
                return new VerifyResult
                {
                    Success = false,
                    StatusCode = sendError == EmailDeliveryConfigErrorMessage ? 503 : 400,
                    ErrorMessage = sendError
                };
            }

            return new VerifyResult { Success = true, ErrorMessage = "인증번호가 발송되었습니다." };
        }

        public async Task<VerifyResult> VerifyCodeAsync(string email, string code)
        {
            var redisDb = redisConnection.GetDatabase();
            var redisKey = $"VerifyCode_{email}";

            var savedCode = await redisDb.StringGetAsync(redisKey);
            if (!savedCode.HasValue || savedCode.ToString() != code)
            {
                return new VerifyResult { Success = false, StatusCode = 400, ErrorMessage = "인증번호가 일치하지 않거나 만료되었습니다." };
            }

            var account = await dbContext.Accounts
                .SingleOrDefaultAsync(a => a.Email == email && a.AccountState == EAccountState.EmailUnverified);

            if (account == null)
            {
                return new VerifyResult { Success = false, StatusCode = 400, ErrorMessage = "이메일 인증에 실패하였습니다." };
            }

            account.AccountState = EAccountState.Active;
            await dbContext.SaveChangesAsync();

            await redisDb.KeyDeleteAsync(redisKey);
            await redisDb.KeyDeleteAsync($"Cooldown_{email}");

            return new VerifyResult { Success = true, Email = email, ErrorMessage = "이메일 인증이 완료되었습니다." };
        }

        public async Task<VerifyResult> ForgotPasswordAsync(string email)
        {
            var account = await dbContext.Accounts.AsNoTracking()
                .SingleOrDefaultAsync(a => a.Email == email);

            if (account == null)
            {
                return new VerifyResult { Success = true, ErrorMessage = "계정이 존재한다면 비밀번호 재설정 메일이 발송되었습니다." };
            }

            if (account.AccountState == EAccountState.EmailUnverified)
            {
                return new VerifyResult { Success = false, StatusCode = 400, ErrorMessage = "이메일 인증이 완료되지 않은 계정입니다." };
            }

            var sendError = await SendResetCodeInternalAsync(email);
            if (sendError != null)
            {
                return new VerifyResult
                {
                    Success = false,
                    StatusCode = sendError == EmailDeliveryConfigErrorMessage ? 503 : 400,
                    ErrorMessage = sendError
                };
            }

            return new VerifyResult { Success = true, ErrorMessage = "이메일 인증번호가 발송되었습니다. 비밀번호 변경 시 인증번호가 필요합니다." };
        }

        public async Task<VerifyResult> ResetPasswordAsync(string email, string code, string newPassword)
        {
            var redisDb = redisConnection.GetDatabase();
            var redisKey = $"ResetCode_{email}";

            var savedCode = await redisDb.StringGetAsync(redisKey);
            if (!savedCode.HasValue || savedCode.ToString() != code)
            {
                return new VerifyResult { Success = false, StatusCode = 400, ErrorMessage = "인증번호가 일치하지 않거나 만료되었습니다." };
            }

            var account = await dbContext.Accounts
                .SingleOrDefaultAsync(a => a.Email == email);

            if (account == null)
            {
                return new VerifyResult { Success = false, StatusCode = 404, ErrorMessage = "계정을 찾을 수 없습니다." };
            }

            account.PasswordHash = BCrypt.Net.BCrypt.HashPassword(newPassword);
            await dbContext.SaveChangesAsync();

            await redisDb.KeyDeleteAsync(redisKey);
            await redisDb.KeyDeleteAsync($"ResetCooldown_{email}");

            return new VerifyResult { Success = true, Email = email, ErrorMessage = "비밀번호가 성공적으로 변경되었습니다. 새로운 비밀번호로 로그인해주세요." };
        }

        private static LoginResult? ValidateAccountState(Account account)
        {
            return account.AccountState switch
            {
                EAccountState.Suspended => new LoginResult
                {
                    Success = false, StatusCode = 403,
                    ErrorCode = "ACCOUNT_SUSPENDED", Email = account.Email,
                    ErrorMessage = "일시 정지된 계정입니다."
                },
                EAccountState.Banned => new LoginResult
                {
                    Success = false, StatusCode = 403,
                    ErrorCode = "ACCOUNT_BANNED", Email = account.Email,
                    ErrorMessage = "영구 정지된 계정입니다."
                },
                EAccountState.EmailUnverified => new LoginResult
                {
                    Success = false, StatusCode = 403,
                    ErrorCode = "EMAIL_UNVERIFIED", Email = account.Email,
                    ErrorMessage = "이메일 인증이 완료되지 않은 계정입니다. 인증 페이지로 이동합니다."
                },
                EAccountState.PendingUnregister => new LoginResult
                {
                    Success = false, StatusCode = 403,
                    ErrorCode = "ACCOUNT_PENDING_UNREGISTER", Email = account.Email,
                    ErrorMessage = "탈퇴한 회원입니다."
                },
                _ => null
            };
        }

        private async Task<string?> SendVerifyCodeInternalAsync(string email)
        {
            if (!IsEmailDeliveryConfigured())
            {
                logger.LogError("AuthService::SendVerifyCodeInternalAsync - SMTP config missing or unresolved");
                return EmailDeliveryConfigErrorMessage;
            }

            var redisDb = redisConnection.GetDatabase();
            var cooldownKey = $"Cooldown_{email}";

            if (await redisDb.KeyExistsAsync(cooldownKey))
            {
                return "30초 후에 다시 시도해주세요.";
            }

            var verifyCode = Random.Shared.Next(100000, 1000000).ToString();
            var redisKey = $"VerifyCode_{email}";

            await redisDb.StringSetAsync(redisKey, verifyCode, TimeSpan.FromMinutes(VerifyCodeExpiryMinutes));
            await redisDb.StringSetAsync(cooldownKey, "1", TimeSpan.FromSeconds(CooldownSeconds));

            const string emailSubject = "[DH1_MMORPG] 회원가입 인증번호 안내";
            var emailBody = $"안녕하세요.\n회원가입 인증번호는 [{verifyCode}] 입니다.\n3분 이내에 입력해주세요.";
            await emailQueue.QueueEmailAsync(new EmailJob(email, emailSubject, emailBody));

            return null;
        }

        private async Task<string?> SendResetCodeInternalAsync(string email)
        {
            if (!IsEmailDeliveryConfigured())
            {
                logger.LogError("AuthService::SendResetCodeInternalAsync - SMTP config missing or unresolved");
                return EmailDeliveryConfigErrorMessage;
            }

            var redisDb = redisConnection.GetDatabase();
            var cooldownKey = $"ResetCooldown_{email}";

            if (await redisDb.KeyExistsAsync(cooldownKey))
            {
                return "30초 후에 다시 시도해주세요.";
            }

            var resetCode = Random.Shared.Next(100000, 1000000).ToString();
            var redisKey = $"ResetCode_{email}";

            await redisDb.StringSetAsync(redisKey, resetCode, TimeSpan.FromMinutes(VerifyCodeExpiryMinutes));
            await redisDb.StringSetAsync(cooldownKey, "1", TimeSpan.FromSeconds(CooldownSeconds));

            const string emailSubject = "[DH1_MMORPG] 비밀번호 재설정 안내";
            var emailBody = $"안녕하세요.\n비밀번호 재설정 인증번호는 [{resetCode}] 입니다.\n5분 이내에 입력해주세요.";
            await emailQueue.QueueEmailAsync(new EmailJob(email, emailSubject, emailBody));

            return null;
        }

        private bool IsEmailDeliveryConfigured()
        {
            if (configuration == null)
            {
                // Unit tests may instantiate AuthService without IConfiguration.
                return true;
            }

            var senderEmail = configuration["SmtpSettings:SenderEmail"];
            var appPassword = configuration["SmtpSettings:AppPassword"];
            var smtpHost = configuration["SmtpSettings:Host"];

            return IsResolvedValue(senderEmail) && IsResolvedValue(appPassword) && IsResolvedValue(smtpHost);
        }

        private static bool IsResolvedValue(string? value)
        {
            if (string.IsNullOrWhiteSpace(value))
            {
                return false;
            }

            return !value.Contains("${", StringComparison.Ordinal);
        }
    }
}
