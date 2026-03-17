using System.Threading.Channels;

namespace LoginServer.Services
{
    // 큐에 담을 이메일 데이터 규격
    public record EmailJob(string ToEmail, string Subject, string Body);

    public interface IEmailQueue
    {
        ValueTask QueueEmailAsync(EmailJob job);
        ValueTask<EmailJob> DequeueEmailAsync(CancellationToken cancellationToken);
    }

    public class EmailQueue : IEmailQueue
    {
        private readonly Channel<EmailJob> mQueue;

        public EmailQueue()
        {
            // 용량 제한이 없는 큐 생성 (필요에 따라 Bounded로 제한 가능)
            var options = new UnboundedChannelOptions { SingleReader = true };
            mQueue = Channel.CreateUnbounded<EmailJob>(options);
        }

        public async ValueTask QueueEmailAsync(EmailJob job)
        {
            await mQueue.Writer.WriteAsync(job);
        }

        public async ValueTask<EmailJob> DequeueEmailAsync(CancellationToken cancellationToken)
        {
            return await mQueue.Reader.ReadAsync(cancellationToken);
        }
    }
}