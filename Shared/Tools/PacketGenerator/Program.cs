using PacketGenerator;

string configPath;
string protoPath;
string prjBasePath;

if (args.Length >= 3)
{
    configPath = args[0];
    protoPath = args[1];
    prjBasePath = args[2];
}
else
{
    var baseDir = AppContext.BaseDirectory;
    configPath = Path.GetFullPath(Path.Combine(baseDir, @"..\..\..\Tools\PacketGenerator\Config\PacketConfig.json"));
    protoPath = Path.GetFullPath(Path.Combine(baseDir, @"..\..\..\..\Shared\Protocol\Proto"));
    prjBasePath = Path.GetFullPath(Path.Combine(baseDir, @"..\..\..\..\"));
}

Console.WriteLine($"[Config Path] {configPath}");
Console.WriteLine($"[Proto Path] {protoPath}");
Console.WriteLine($"[prjBasePath] {prjBasePath}");

// Config 로드 및 예외 처리
PacketProjectsConfig? resultConfig = null;

try
{
    resultConfig = PacketConfig.Load(configPath);
}
catch (FileNotFoundException)
{
    Console.WriteLine($"[Error] 설정 파일을 찾을 수 없습니다. 경로를 확인하세요: {configPath}");
    Environment.Exit(1);
}
catch (Exception ex)
{
    Console.WriteLine($"[Error] PacketConfig.json 파싱 중 오류가 발생했습니다!");
    Console.WriteLine($"상세 내용: {ex.Message}");
    Environment.Exit(1);
}

Console.WriteLine(PacketHandlerGenerator.Generate(resultConfig, protoPath, prjBasePath)
    ? "SUCCESS"
    : "FAILED to generate packet handler.");