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
    configPath = Path.GetFullPath(Path.Combine(baseDir, @"..\..\..\Config\Tools\PacketGeneratorConfig.json"));
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

var handlers = Parser.ParseHandlersFromDesc(protoPath);

EnumGenerator.GenerateSharedEnum(handlers, Path.Combine(prjBasePath, @"Shared\Protocol\PacketId\PacketId.h"));

if (resultConfig.Projects.Count == 0)
{
    Console.WriteLine("[Warning] No projects configured in config file — nothing to generate.");
}

foreach (var prjConfig in resultConfig.Projects)
{
    var outputPath = Path.Combine(prjBasePath, prjConfig.Name, "PacketHandler");

    HandlerGenerator.GenerateCpps(handlers, prjConfig.Role, outputPath);
}

// 클라이언트 프로토콜 복사 (.pb.h / .pb.cpp → UE 모듈)
if (!string.IsNullOrEmpty(resultConfig.ClientProtocolPath))
{
    var protoSourceDir = Path.Combine(prjBasePath, @"Shared\Protocol");
    var clientProtocolDir = Path.Combine(prjBasePath, resultConfig.ClientProtocolPath);
    ClientProtocolCopier.CopyClientProtocolHeaders(protoSourceDir, clientProtocolDir, handlers, "CLIENT");

    // PacketId.h도 클라이언트에 복사
    var packetIdSource = Path.Combine(prjBasePath, @"Shared\Protocol\PacketId\PacketId.h");
    var packetIdDestDir = Path.Combine(clientProtocolDir, "PacketId");
    if (!Directory.Exists(packetIdDestDir))
    {
        Directory.CreateDirectory(packetIdDestDir);
    }

    if (File.Exists(packetIdSource))
    {
        File.Copy(packetIdSource, Path.Combine(packetIdDestDir, "PacketId.h"), true);
        Console.WriteLine($"[ClientProtocol] Copied PacketId.h to: {packetIdDestDir}");
    }
}
