using MailKit.Net.Smtp;
using MimeKit;

namespace LoginServer.Services
{
    public class EmailBackgroundService(IEmailQueue emailQueue, IConfiguration smtpConfig, ILogger<EmailBackgroundService> logger) : BackgroundService
    {
        private IEmailQueue EmailQueue { get; } = emailQueue;
        private IConfiguration SmtpConfig { get; } = smtpConfig;
        private ILogger<EmailBackgroundService> Logger { get; } = logger;

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                try
                {
                    // 큐에 데이터가 들어올 때까지 대기
                    var job = await EmailQueue.DequeueEmailAsync(stoppingToken);
                    await SendEmailAsync(job, stoppingToken);
                }
                catch (OperationCanceledException)
                {
                    // 서버 종료 시 발생하는 정상적인 예외
                    break;
                }
                catch (Exception ex)
                {
                    Logger.LogError(ex, "이메일 발송 중 오류가 발생했습니다.");
                }
            }
        }

        private async Task SendEmailAsync(EmailJob job, CancellationToken token)
        {
            var message = new MimeMessage();
            message.From.Add(new MailboxAddress(SmtpConfig["SmtpSettings:SenderName"], SmtpConfig["SmtpSettings:SenderEmail"] ?? string.Empty));
            message.To.Add(new MailboxAddress("", job.ToEmail));
            message.Subject = job.Subject;
            message.Body = new TextPart("plain") { Text = job.Body };

            using var client = new SmtpClient();
            await client.ConnectAsync(SmtpConfig["SmtpSettings:Host"], int.Parse(SmtpConfig["SmtpSettings:Port"]!), MailKit.Security.SecureSocketOptions.StartTls, token);
            await client.AuthenticateAsync(SmtpConfig["SmtpSettings:SenderEmail"], SmtpConfig["SmtpSettings:AppPassword"], token);
            await client.SendAsync(message, token);
            await client.DisconnectAsync(true, token);
        }
    }
}
