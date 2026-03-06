using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Text.Json;
using JetBrains.Annotations;

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
    }

    internal class PacketConfig
    {
        public static PacketProjectsConfig Load(string configFilePath)
        {
            if (!File.Exists(configFilePath))
            {
                Console.WriteLine($"[Error] 설정 파일을 찾을 수 없습니다: {configFilePath}");
                return new PacketProjectsConfig(); // 파일이 없으면 빈 객체 반환
            }

            try
            {
                var jsonString = File.ReadAllText(configFilePath);
                var config = JsonSerializer.Deserialize<PacketProjectsConfig>(jsonString, mOptions);
                return config ?? new PacketProjectsConfig();
            }
            catch (JsonException ex)
            {
                Console.WriteLine($"[Error] JSON 파싱 실패: {ex.Message}");
                return new PacketProjectsConfig();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[Error] 파일 읽기 실패: {ex.Message}");
                return new PacketProjectsConfig();
            }
        }

        public static JsonSerializerOptions mOptions = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true
        };
    }
}
