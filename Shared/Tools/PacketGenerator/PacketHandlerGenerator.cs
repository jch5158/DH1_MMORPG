using Google.Protobuf.Reflection;
using Google.Protobuf;

namespace PacketGenerator
{
    internal class PacketHandlerGenerator
    {
        public static bool Generate(PacketProjectsConfig config, string protoPath, string prjBasePath)
        {
            Path.Combine(protoPath + "Enum.desc");

            var descBytes = File.ReadAllBytes(Path.Combine(protoPath + @"/Enum.desc")); // 예: "Protocol.desc"
            var descriptorSet = FileDescriptorSet.Parser.ParseFrom(descBytes);

            var eRoleDescriptor = descriptorSet.File
                .SelectMany(f => f.EnumType)
                .FirstOrDefault(e => e.Name == "eRole");

            if (eRoleDescriptor == null)
            {
                Console.WriteLine("[Error] .desc 파일에서 'eRole' Enum을 찾을 수 없습니다.");
                return false;
            }

            var validRoles = eRoleDescriptor.Value.Select(v => v.Name).ToHashSet(StringComparer.OrdinalIgnoreCase);

            foreach (var projectReceiver in config.Projects)
            {
                foreach (var projectSender in config.Projects)
                {
                    var receiverRole = projectReceiver.Role.ToUpper();
                    var senderRole = projectSender.Role.ToUpper();

                    if (!validRoles.Contains(receiverRole) || !validRoles.Contains(senderRole))
                    {
                        Console.WriteLine($"[Error] 프로젝트의 역할이 유효하지 않습니다. Receiver: {projectReceiver.Role}, Sender: {projectSender.Role}");
                        return false;
                    }

                    if (string.Equals(projectSender.Role, projectReceiver.Role, StringComparison.OrdinalIgnoreCase))
                    {
                        continue;
                    }

                    var outputPath = Path.Combine(prjBasePath, @$"{projectReceiver.Name}\PacketHandler");
                    if (!GenerateHandlerFile(receiverRole, senderRole, protoPath, outputPath))
                    {
                        Console.WriteLine("GenerateHandlerFile is Failed");
                        return false;
                    }

                    // ReSharper disable once InvertIf
                    if (!GenerateServiceTypeHandlerFile(receiverRole, senderRole, protoPath, outputPath))
                    {
                        Console.WriteLine("GenerateServiceTypeHandlerFile is Failed");
                        return false;
                    }
                }
            }

            return true;
        }

        public static bool GenerateHandlerFile(string receiver, string sender, string protoDirPath, string outputDirPath)
        {
            var descFiles = Directory.GetFiles(protoDirPath, "*.desc");
            if (descFiles.Length == 0)
            {
                Console.WriteLine($"[Error] '{protoDirPath}' 경로에서 .desc 파일을 찾을 수 없습니다.");
                return false;
            }

            foreach (var protoFilePath in descFiles)
            {
                var protoName = Path.GetFileNameWithoutExtension(protoFilePath);
                if (protoName is "Enum" or "PacketId" or "Struct")
                {
                    continue;
                }

                var fileName = $"{protoName}PacketHandler.h";
                var outputFilePath = Path.Combine(outputDirPath, fileName);

                if (!GenerateHandleFileContent(receiver, sender, protoName, protoFilePath, out var handleFileContent))
                {
                    return false;
                }

                try
                {
                    var directoryPath = Path.GetDirectoryName(outputFilePath);
                    if (!string.IsNullOrEmpty(directoryPath) && !Directory.Exists(directoryPath))
                    {
                        Directory.CreateDirectory(directoryPath);
                    }

                    File.WriteAllText(outputFilePath, handleFileContent, new System.Text.UTF8Encoding(true));
                }
                catch (Exception e)
                {
                    Console.WriteLine($"[Generate] 패킷 핸들러 생성 중 오류 발생: {e.Message}");
                    return false;
                }
            }

            return true;
        }

        private static bool GenerateHandleFileContent(string receiver, string sender, string protoName, string filePath,
            out string handleFileContent)
        {
            handleFileContent = "";

            try
            {
                // 1. 레지스트리 의존성 완전 제거: 순수 .desc 파일만 파싱
                using var stream = File.OpenRead(filePath);
                var descriptorSet = FileDescriptorSet.Parser.ParseFrom(stream);

                // 2. 동적으로 eRole의 (정수 -> 대문자 문자열) 매핑 딕셔너리 생성
                var eRoleDescriptor = descriptorSet.File
                    .SelectMany(f => f.EnumType)
                    .FirstOrDefault(e => e.Name == "eRole");

                if (eRoleDescriptor == null)
                {
                    return false;
                }

                var roleMap = eRoleDescriptor.Value.ToDictionary(v => v.Number, v => v.Name.ToUpper());

                var senderExt = descriptorSet.File.SelectMany(f => f.Extension).FirstOrDefault(e => e.Name == "sender");
                var receiverExt = descriptorSet.File.SelectMany(f => f.Extension).FirstOrDefault(e => e.Name == "receiver");
                if (senderExt == null || receiverExt == null)
                {
                    Console.WriteLine("[Error] .desc에서 sender 또는 receiver 확장을 찾을 수 없습니다.");
                    return false;
                }

                var senderFieldNum = senderExt.Number;
                var receiverFieldNum = receiverExt.Number;

                var initHandleString = string.Empty;
                var handleFunctionDeclareString = string.Empty;
                var makeSendBufferFunctionString = string.Empty;

                foreach (var fileProto in descriptorSet.File)
                {
                    if (!fileProto.Name.EndsWith($"{protoName}.proto"))
                    {
                        continue;
                    }

                    foreach (var msg in fileProto.MessageType)
                    {
                        if (msg.Options == null)
                        {
                            continue;
                        }

                        var optionsBytes = msg.Options.ToByteArray();
                        var input = new CodedInputStream(optionsBytes);

                        var currentSenderVal = -1;
                        var currentReceiverVal = -1;

                        while (!input.IsAtEnd)
                        {
                            var tag = input.ReadTag();
                            var fieldNum = WireFormat.GetTagFieldNumber(tag);

                            if (fieldNum == senderFieldNum)
                            {
                                currentSenderVal = input.ReadEnum();
                            }
                            else if (fieldNum == receiverFieldNum)
                            {
                                currentReceiverVal = input.ReadEnum();
                            }
                            else
                            {
                                input.SkipLastField(); // 필요 없는 옵션은 스킵
                            }
                        }

                        if (currentSenderVal == -1 || currentReceiverVal == -1)
                        {
                            continue;
                        }

                        if (!roleMap.TryGetValue(currentSenderVal, out var senderName) ||
                            !roleMap.TryGetValue(currentReceiverVal, out var receiverName))
                        {
                            continue;
                        }

                        if (sender == senderName && receiver == receiverName)
                        {
                            var packetName = msg.Name;
                            initHandleString += string.Format(PacketFormatter.HANDLE_INIT_FORMAT, $"ID_{packetName}",
                                packetName);

                            handleFunctionDeclareString +=
                                string.Format(PacketFormatter.HANDLE_DECLARE_FORMAT, packetName);
                        }

                        if (receiver == senderName && sender == receiverName)
                        {
                            var packetName = msg.Name;
                            makeSendBufferFunctionString += string.Format(PacketFormatter.MAKE_SEND_BUFFER_FORMAT,
                                packetName, $"ID_{packetName}");
                        }
                    }
                }

                handleFileContent = string.Format(PacketFormatter.HANDLE_FILE_FORMAT, protoName,
                    initHandleString,
                    handleFunctionDeclareString,
                    makeSendBufferFunctionString);

                handleFileContent = handleFileContent.Replace("\r\n", "\n").Replace("\n", "\r\n");
            }
            catch (Exception e)
            {
                Console.WriteLine($"[GenerateInitHandleString] 오류 발생: {e.Message}");
                return false;
            }

            return true;
        }


        public static bool GenerateServiceTypeHandlerFile(string receiver, string sender, string protoDirPath, string outputDirPath)
        {
            var descFiles = Directory.GetFiles(protoDirPath, "*.desc");
            if (descFiles.Length == 0)
            {
                Console.WriteLine($"[Error] '{protoDirPath}' 경로에서 .desc 파일을 찾을 수 없습니다.");
                return false;
            }

            const string fileName = "PacketServiceTypeHandler.h";
            var outputFilePath = Path.Combine(outputDirPath, fileName);

            var includeString = string.Empty;
            var initHandlerString = string.Empty;
            var handleInitString = string.Empty;

            foreach (var protoFilePath in descFiles)
            {
                var protoName = Path.GetFileNameWithoutExtension(protoFilePath);
                if (protoName is "Enum" or "Struct")
                {
                    continue;
                }

                if (protoName is "PacketId")
                {
                    if (GenerateHandleInitString(protoFilePath, ref handleInitString))
                    {
                        continue;
                    }

                    Console.WriteLine("GenerateFile is Failed");
                    return false;
                }

                includeString += string.Format(PacketFormatter.SERVICE_TYPE_INCLUDE_FORMAT, protoName);
                initHandlerString += string.Format(PacketFormatter.SERVICE_TYPE_INIT_FORMAT, protoName);
            }

            var serviceTypeHandlerContent = string.Format(PacketFormatter.HANDLE_SERVICE_TYPE_FILE_FORMAT,
                includeString,
                initHandlerString,
                handleInitString);

            try
            {
                var directoryPath = Path.GetDirectoryName(outputFilePath);

                if (!string.IsNullOrEmpty(directoryPath) && !Directory.Exists(directoryPath))
                {
                    Directory.CreateDirectory(directoryPath);
                }

                serviceTypeHandlerContent = serviceTypeHandlerContent.Replace("\r\n", "\n").Replace("\n", "\r\n");

                File.WriteAllText(outputFilePath, serviceTypeHandlerContent, new System.Text.UTF8Encoding(true));
            }
            catch (Exception e)
            {
                Console.WriteLine($"[GenerateServiceTypeHandlerFile] 패킷 핸들러 생성 중 오류 발생: {e.Message}");
                return false;
            }

            return true;
        }

        public static bool GenerateHandleInitString(string filePath, ref string handleInitString)
        {
            try
            {
                using var stream = File.OpenRead(filePath);
                var descriptorSet = FileDescriptorSet.Parser.ParseFrom(stream);

                var handlerNameExt = descriptorSet.File
                    .SelectMany(f => f.Extension)
                    .FirstOrDefault(e => e.Name.ToLower() == "handler_name");

                if (handlerNameExt == null)
                {
                    Console.WriteLine("[Error] .desc에서 handler_name(또는 HandlerName) 확장을 찾을 수 없습니다.");
                    return false;
                }

                var handlerNameFieldNum = handlerNameExt.Number;

                foreach (var fileProto in descriptorSet.File)
                {
                    foreach (var enumType in fileProto.EnumType)
                    {
                        foreach (var enumValue in enumType.Value)
                        {
                            if (enumValue.Options == null) continue;

                            var optionsBytes = enumValue.Options.ToByteArray();
                            var input = new CodedInputStream(optionsBytes);

                            string? handlerName = null;

                            while (!input.IsAtEnd)
                            {
                                var tag = input.ReadTag();
                                var fieldNum = WireFormat.GetTagFieldNumber(tag);

                                if (fieldNum == handlerNameFieldNum)
                                {
                                    handlerName = input.ReadString();
                                }
                                else
                                {
                                    input.SkipLastField();
                                }
                            }

                            if (string.IsNullOrEmpty(handlerName))
                            {
                                continue;
                            }

                            var enumName = enumValue.Name;
                            handleInitString += string.Format(
                                PacketFormatter.SERVICE_TYPE_HANDLE_INIT_FORMAT,
                                enumName,
                                handlerName
                            );
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[GenerateHandleInitString] 패킷 핸들러 생성 중 오류 발생: {e.Message}");
                return false;
            }

            return true;
        }
    }
}
