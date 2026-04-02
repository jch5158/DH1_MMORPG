using System.Text.Json;

namespace PacketGenerator
{
    public class ProjectConfig
    {
        public string Name { get; set; } = string.Empty;
        public string Role { get; set; } = string.Empty;
    }

    public class PacketProjectsConfig
    {
        public List<ProjectConfig> Projects { get; set; } = [];
        public string ClientProtocolPath { get; set; } = string.Empty;
    }

    internal class PacketConfig
    {
        public static PacketProjectsConfig Load(string configFilePath)
        {
            var jsonString = File.ReadAllText(configFilePath);
            var config = JsonSerializer.Deserialize<PacketProjectsConfig>(jsonString, s_jsonOptions);
            return config ?? throw new InvalidOperationException(
                $"JSON deserialization returned null for: {configFilePath}");
        }

        private static readonly JsonSerializerOptions s_jsonOptions = new()
        {
            PropertyNameCaseInsensitive = true
        };
    }
}
