using PacketGenerator;

var baseDirPath = System.AppContext.BaseDirectory;
var configDirPath = Path.Combine(baseDirPath, @"..\Config\PacketConfig.json");
var resultConfig = PacketConfig.Load(configDirPath);

if (PacketHandlerGenerator.Generate(resultConfig, baseDirPath))
{
    Console.WriteLine("SUCCESS");
}

Thread.Sleep(10000);