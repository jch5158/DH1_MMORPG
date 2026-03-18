using Google.Protobuf;
using Google.Protobuf.Reflection;
using System.Data;

namespace PacketGenerator
{
    public class PacketInfo
    {
        public string MessageName { get; set; } = string.Empty;
        public uint PacketId { get; set; }
        public string Sender { get; set; } = string.Empty;
        public string Receiver { get; set; } = string.Empty;
    }

    public class HandlerInfo
    {
        public string ProtoFileName { get; set; } = string.Empty;
        public string HandlerName { get; set; } = string.Empty;
        public string ServiceTypeName { get; set; } = string.Empty;
        public List<PacketInfo> Packets { get; set; } = [];
    }

    public static class Parser
    {
        public static List<HandlerInfo> ParseHandlersFromDesc(string protocolDirPath)
        {
            var handlers = new List<HandlerInfo>();

            var descFiles = Directory.GetFiles(protocolDirPath, "*.desc");
            if (descFiles.Length == 0)
            {
                Console.WriteLine($"[Error] '{protocolDirPath}' 경로에서 .desc 파일을 찾을 수 없습니다.");
                return handlers;
            }

            var serviceTypeDescriptor = getEnumDescriptorProto("eRole", protocolDirPath);
            var roleDescriptor = getEnumDescriptorProto("eServiceType", protocolDirPath); ;
            if (serviceTypeDescriptor == null || roleDescriptor == null)
            {
                return handlers;
            }

            string[] ignoreDescFiles = ["Enum.desc", "Struct.desc", "PacketOption.desc"];
            foreach (var descFile in descFiles)
            {
                var fileName = Path.GetFileName(descFile);
                if (ignoreDescFiles.Contains(fileName, StringComparer.OrdinalIgnoreCase))
                {
                    continue;
                }

                using var stream = File.OpenRead(descFile);
                var descriptorSet = FileDescriptorSet.Parser.ParseFrom(stream);

                var protoName = fileName.Replace(".desc", ".proto");
                var protoFile = descriptorSet.File.FirstOrDefault(f => Path.GetFileName(f.Name) == protoName);
                if (protoFile == null)
                {
                    continue;
                }

                // 1. 파일 옵션 파싱 (HandlerInfo 세팅)
                var handlerInfo = new HandlerInfo
                {
                    ProtoFileName = Path.GetFileNameWithoutExtension(protoName) // 확장자 뺀 이름 (예: "Echo")
                };

                if (protoFile.Options != null)
                {
                    var optionsBytes = protoFile.Options.ToByteArray();
                    var inputStream = new CodedInputStream(optionsBytes);
                    uint tag;
                    while ((tag = inputStream.ReadTag()) != 0)
                    {
                        var fieldNum = WireFormat.GetTagFieldNumber(tag);
                        switch (fieldNum)
                        {
                            case 50001:
                                var svcTypeValue = inputStream.ReadInt32();
                                handlerInfo.ServiceTypeName =
                                    serviceTypeDescriptor?.Value.FirstOrDefault(v => v.Number == svcTypeValue)
                                        ?.Name ?? string.Empty;
                                break;
                            case 50002:
                                handlerInfo.HandlerName = inputStream.ReadString();
                                break;
                            default:
                                inputStream.SkipLastField();
                                break;
                        }
                    }
                }

                if (string.IsNullOrEmpty(handlerInfo.HandlerName) ||
                    string.IsNullOrEmpty(handlerInfo.ServiceTypeName))
                {
                    continue;
                }

                foreach (var message in protoFile.MessageType)
                {
                    if (message.Options == null)
                    {
                        continue;
                    }

                    var packetInfo = new PacketInfo
                    {
                        MessageName = message.Name
                    };

                    var optionsBytes = message.Options.ToByteArray();
                    var inputStream = new CodedInputStream(optionsBytes);
                    uint tag;
                    while ((tag = inputStream.ReadTag()) != 0)
                    {
                        var fieldNum = WireFormat.GetTagFieldNumber(tag);
                        switch (fieldNum)
                        {
                            case 50003:
                                packetInfo.PacketId = inputStream.ReadUInt32();
                                break;
                            case 50004:
                                var senderValue = inputStream.ReadInt32();
                                packetInfo.Sender =
                                    roleDescriptor?.Value.FirstOrDefault(v => v.Number == senderValue)?.Name ??
                                    string.Empty;
                                break;
                            case 50005:
                                var receiverValue = inputStream.ReadInt32();
                                packetInfo.Receiver =
                                    roleDescriptor?.Value.FirstOrDefault(v => v.Number == receiverValue)?.Name ??
                                    string.Empty;
                                break;
                            default:
                                inputStream.SkipLastField();
                                break;
                        }
                    }

                    if (packetInfo.PacketId > 0 && !string.IsNullOrEmpty(packetInfo.Sender) &&
                        !string.IsNullOrEmpty(packetInfo.Receiver))
                    {
                        handlerInfo.Packets.Add(packetInfo);
                    }
                }

                if (handlerInfo.Packets.Count > 0)
                {
                    handlers.Add(handlerInfo);
                }
            }

            return handlers;
        }

        private static EnumDescriptorProto? getEnumDescriptorProto(string enumName, string protocolDirPath)
        {
            var enumDescPath = Path.Combine(protocolDirPath, "Enum.desc");
            if (!File.Exists(enumDescPath))
            {
                return null;
            }

            using var stream = File.OpenRead(enumDescPath);
            var descriptorSet = FileDescriptorSet.Parser.ParseFrom(stream);
            var enumFile = descriptorSet.File.FirstOrDefault(f => Path.GetFileName(f.Name) == "Enum.proto");
            return enumFile?.EnumType.FirstOrDefault(e => e.Name == enumName);
        }
    }
}