using PacketGenerator;

string configPath;
string outputDirPath;
string protoPath;
string prjBasePath;

if (args.Length >= 4)
{
    configPath = args[0];
    outputDirPath = args[1];
    protoPath = args[2];
    prjBasePath = args[3];
}
else
{
    var baseDir = AppContext.BaseDirectory;
    configPath = Path.GetFullPath(Path.Combine(baseDir, @"..\..\..\Tools\PacketGenerator\Config\PacketConfig.json"));
    outputDirPath = Path.GetFullPath(Path.Combine(baseDir, @"..\..\..\PacketGenerator\Generated"));
    protoPath = Path.GetFullPath(Path.Combine(baseDir, @"..\..\..\..\Shared\Protocol"));
    prjBasePath = Path.GetFullPath(Path.Combine(baseDir, @"..\..\..\..\"));
}

Console.WriteLine($"[Config Path] {configPath}");
Console.WriteLine($"[Output Path] {outputDirPath}");
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

Console.WriteLine(PacketHandlerGenerator.Generate(resultConfig, outputDirPath, protoPath, prjBasePath)
    ? "SUCCESS"
    : "FAILED to generate packet handler.");