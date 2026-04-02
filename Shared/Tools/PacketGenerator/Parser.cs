using Google.Protobuf;
using Google.Protobuf.Reflection;

namespace PacketGenerator
{
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

            var serviceTypeDescriptor = GetEnumDescriptorProto("eServiceType", protocolDirPath);
            var roleDescriptor = GetEnumDescriptorProto("eRole", protocolDirPath);
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

                uint autoPacketId = 0;
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
                                // packet_id is now auto-assigned, skip if present for backward compatibility
                                inputStream.ReadUInt32();
                                break;
                            case 50004:
                                var senderValue = inputStream.ReadInt32();
                                packetInfo.Sender =
                                    roleDescriptor?.Value.FirstOrDefault(v => v.Number == senderValue)?.Name ??
                                    string.Empty;
                                break;
                            case 50005:
                                if (WireFormat.GetTagWireType(tag) == WireFormat.WireType.LengthDelimited)
                                {
                                    var length = inputStream.ReadLength();
                                    var limit = inputStream.Position + length;

                                    while (inputStream.Position < limit)
                                    {
                                        var receiverValue = inputStream.ReadInt32();
                                        var receiverName = roleDescriptor?.Value.FirstOrDefault(v => v.Number == receiverValue)?.Name;
                                        if (!string.IsNullOrEmpty(receiverName)) packetInfo.Receivers.Add(receiverName);
                                    }
                                }
                                else
                                {
                                    var receiverValue = inputStream.ReadInt32();
                                    var receiverName = roleDescriptor?.Value.FirstOrDefault(v => v.Number == receiverValue)?.Name;
                                    if (!string.IsNullOrEmpty(receiverName)) packetInfo.Receivers.Add(receiverName);
                                }
                                break;
                            default:
                                inputStream.SkipLastField();
                                break;
                        }
                    }

                    if (!string.IsNullOrEmpty(packetInfo.Sender) && packetInfo.Receivers.Count > 0)
                    {
                        // Auto-assign packet ID based on message order (1-based)
                        packetInfo.PacketId = ++autoPacketId;
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

        private static EnumDescriptorProto? GetEnumDescriptorProto(string enumName, string protocolDirPath)
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
