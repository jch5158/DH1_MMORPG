using System.Text;

namespace PacketGenerator
{
    public static class EnumGenerator
    {
        public static void GenerateSharedEnum(List<HandlerInfo> handlers, string outputFilePath)
        {
            var enumsBuilder = new StringBuilder();

            var groupedByProto = handlers.GroupBy(h => h.ProtoFileName);
            foreach (var group in groupedByProto)
            {
                var protoName = group.Key;
                var membersBuilder = new StringBuilder();
                var idSet = new HashSet<uint>();

                foreach (var handler in group)
                {
                    foreach (var packet in handler.Packets)
                    {
                        if (!idSet.Add(packet.PacketId))
                        {
                            Console.WriteLine($"[Error] 패킷 ID 중복 발생! 파일 생성 중단. Proto: {protoName}, ID: {packet.PacketId}, Message: {packet.MessageName}");
                            return;
                        }

                        membersBuilder.AppendLine($"        {packet.MessageName} = {packet.PacketId},");
                    }
                }

                // 3. Enum 블록 포맷팅
                enumsBuilder.AppendLine();
                enumsBuilder.AppendLine($"\tenum e{protoName}PacketId : uint16");
                enumsBuilder.AppendLine("\t{");
                enumsBuilder.Append(membersBuilder);
                enumsBuilder.AppendLine("\t};");
                enumsBuilder.AppendLine();
            }

            var fileContent = string.Format(PacketFormatter.ENUM_PACKET_ID_FORMAT, enumsBuilder.ToString());
            var normalizedContent = PacketFormatter.NormalizeToCrlf(fileContent);

            var directoryPath = Path.GetDirectoryName(outputFilePath);
            if (!string.IsNullOrEmpty(directoryPath) && !Directory.Exists(directoryPath))
            {
                Directory.CreateDirectory(directoryPath);
            }

            File.WriteAllText(outputFilePath, normalizedContent, new UTF8Encoding(true));
        }
    }
}
