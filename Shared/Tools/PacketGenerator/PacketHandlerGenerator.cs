using Google.Protobuf;
using Google.Protobuf.Reflection;
using System.Data;
using System.Text;

namespace PacketGenerator
{
    public class PacketInfo
    {
        public string MessageName { get; set; } = string.Empty;
        public uint PacketId { get; set; }
        public string Sender { get; set; } = string.Empty;
        public List<string> Receivers { get; set; } = []; // List로 변경
    }

    public class HandlerInfo
    {
        public string ProtoFileName { get; set; } = string.Empty;
        public string HandlerName { get; set; } = string.Empty;
        public string ServiceTypeName { get; set; } = string.Empty;
        public List<PacketInfo> Packets { get; set; } = [];
    }

    public static class RoleHelper
    {
        private const string ServerRole = "SERVER";

        private static readonly HashSet<string> NonServerRoles = new(StringComparer.OrdinalIgnoreCase)
        {
            "CLIENT", "ECHO_CLIENT", "ECHO_SERVER", "ROLE_NONE"
        };

        public static bool IsMatchRole(string packetRole, string targetRole)
        {
            if (packetRole.Equals(ServerRole, StringComparison.OrdinalIgnoreCase))
            {
                return !NonServerRoles.Contains(targetRole);
            }

            return packetRole.Equals(targetRole, StringComparison.OrdinalIgnoreCase);
        }
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

            var serviceTypeDescriptor = getEnumDescriptorProto("eServiceType", protocolDirPath);
            var roleDescriptor = getEnumDescriptorProto("eRole", protocolDirPath); ;
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

    public static class EnumGenerator
    {
        public static void GenerateSharedEnum(List<HandlerInfo> handlers, string outputDirPath)
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

            var directoryPath = Path.GetDirectoryName(outputDirPath);
            if (!string.IsNullOrEmpty(directoryPath) && !Directory.Exists(directoryPath))
            {
                Directory.CreateDirectory(directoryPath);
            }

            File.WriteAllText(outputDirPath, normalizedContent, new UTF8Encoding(true));
        }
    }

    public static class Generator
    {
        public static void GenerateCpps(List<HandlerInfo> handlers, string targetRole, string outputDirPath)
        {
            if (!Directory.Exists(outputDirPath))
            {
                Directory.CreateDirectory(outputDirPath);
            }

            var serviceIncludeBuilder = new StringBuilder();
            var serviceInitBuilder = new StringBuilder();
            var serviceHandleInitBuilder = new StringBuilder();

            foreach (var handler in handlers)
            {
                var handleInitBuilder = new StringBuilder();
                var handleDeclareBuilder = new StringBuilder();
                var makeSendBufferBuilder = new StringBuilder();

                foreach (var packet in handler.Packets)
                {
                    if (packet.Receivers.Any(r => RoleHelper.IsMatchRole(r, targetRole)))
                    {
                        handleInitBuilder.AppendFormat(PacketFormatter.RECEIVE_HANDLE_INIT_FORMAT,
                            handler.ProtoFileName, // {0}
                            packet.MessageName); // {1}

                        handleDeclareBuilder.AppendFormat(PacketFormatter.RECEIVE_HANDLE_DECLARE_FORMAT,
                            packet.MessageName); // {0}
                    }

                    if (RoleHelper.IsMatchRole(packet.Sender, targetRole))
                    {
                        makeSendBufferBuilder.AppendFormat(PacketFormatter.SEND_MAKE_SEND_BUFFER_FORMAT,
                            packet.MessageName, // {0}
                            packet.MessageName); // {1} (상수/Enum 이름으로 가정)
                    }
                }

                if (handleInitBuilder.Length == 0 && makeSendBufferBuilder.Length == 0)
                {
                    continue;
                }

                var handlerContent = string.Format(PacketFormatter.HANDLE_FILE_FORMAT,
                    handler.ProtoFileName, // {0}
                    handler.HandlerName, // {1}
                    handleInitBuilder.ToString(), // {2}
                    handleDeclareBuilder.ToString(), // {3}
                    makeSendBufferBuilder.ToString(),
                    handler.ServiceTypeName); // {4}

                var normalizedHandlerContent = PacketFormatter.NormalizeToCrlf(handlerContent);
                var handlerFilePath = Path.Combine(outputDirPath, $"{handler.HandlerName}.h");
                File.WriteAllText(handlerFilePath, normalizedHandlerContent, new UTF8Encoding(true));

                serviceIncludeBuilder.AppendFormat(PacketFormatter.SERVICE_TYPE_INCLUDE_FORMAT, handler.HandlerName);
                serviceInitBuilder.AppendFormat(PacketFormatter.SERVICE_TYPE_INIT_FORMAT, handler.HandlerName);
                serviceHandleInitBuilder.AppendFormat(PacketFormatter.SERVICE_TYPE_HANDLE_INIT_FORMAT,
                    handler.ServiceTypeName,
                    handler.HandlerName);
            }

            if (serviceInitBuilder.Length <= 0)
            {
                return;
            }

            var dispatcherContent = string.Format(PacketFormatter.HANDLE_SERVICE_TYPE_FILE_FORMAT,
                serviceIncludeBuilder.ToString(),
                serviceInitBuilder.ToString(),
                serviceHandleInitBuilder.ToString());

            var normalizedDispatcherContent = PacketFormatter.NormalizeToCrlf(dispatcherContent);
            var dispatcherFilePath = Path.Combine(outputDirPath, "PacketServiceTypeHandler.h");
            File.WriteAllText(dispatcherFilePath, normalizedDispatcherContent, new UTF8Encoding(true));
        }
    }
}