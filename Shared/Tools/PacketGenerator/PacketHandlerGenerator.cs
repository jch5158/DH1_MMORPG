using Google.Protobuf;
using Google.Protobuf.Reflection;
using Protocol;

namespace PacketGenerator
{
    internal class PacketHandlerGenerator
    {
        public static bool Generate(PacketProjectsConfig config, string outputDirPath, string protoPath, string prjBasePath)
        {
            foreach (var projectReceiver in config.Projects)
            {
                foreach (var projectSender in config.Projects)
                {
                    if (projectReceiver.Name == projectSender.Name)
                    {
                        continue;
                    }

                    if (!Enum.TryParse<eRole>(projectReceiver.Role, true, out var receiver) || !Enum.TryParse<eRole>(projectSender.Role, true, out var sender))
                    {
                        Console.WriteLine($"[Error] 프로젝트 '{projectReceiver.Name}'의 역할 '{projectReceiver.Role}'이(가) 유효하지 않습니다.");
                        return false;
                    }

                    var outputPath = Path.Combine(prjBasePath, @$"{projectReceiver.Name}\Generated");
                    if (!GenerateHandlerFile(receiver, sender, protoPath, outputPath))
                    {
                        Console.WriteLine("GenerateFile is Failed");
                        return false;
                    }

                    if (!GenerateServiceTypeHandlerFile(receiver, sender, protoPath, outputPath))
                    {
                        Console.WriteLine("GenerateFile is Failed");
                        return false;
                    }
                }
            }

            return true;
        }


        public static bool GenerateServiceTypeHandlerFile(eRole receiver, eRole sender, string protoDirPath, string outputDirPath)
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
                    if (GenerateHandleInitString(protoFilePath, ref handleInitString) == false)
                    {
                        Console.WriteLine("GenerateFile is Failed");
                        return false;
                    }
                }
                else
                {
                    GenerateIncludeString(protoName, ref includeString);

                    GenerateServiceTypeInitString(protoName, ref initHandlerString);
                }
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

                File.WriteAllText(outputFilePath, serviceTypeHandlerContent, new System.Text.UTF8Encoding(true));
            }
            catch (Exception e)
            {
                Console.WriteLine($"[GenerateServiceTypeHandlerFile] 패킷 핸들러 생성 중 오류 발생: {e.Message}");
                return false;
            }

            return true;
        }

        public static void GenerateIncludeString(string protoName, ref string includeString)
        {
            includeString += string.Format(PacketFormatter.SERVICE_TYPE_INCLUDE_FORMAT, protoName);
        }

        public static void GenerateServiceTypeInitString(string protoName, ref string initHandlerString)
        {
            initHandlerString += string.Format(PacketFormatter.SERVICE_TYPE_INIT_FORMAT, protoName);
        }

        public static bool GenerateHandleInitString(string filePath, ref string handleInitString)
        {
            try
            {
                // 1. 레지스트리 생성 및 등록 (이 코드가 없으면 무조건 Unknown으로 빠집니다!)
                var registry = new ExtensionRegistry
                {
                    EnumExtensions.Sender,
                    EnumExtensions.Receiver,
                    PacketIdExtensions.HandlerName
                };

                using var stream = File.OpenRead(filePath);
                var descriptorSet = FileDescriptorSet.Parser.WithExtensionRegistry(registry).ParseFrom(stream);
                foreach (var fileProto in descriptorSet.File)
                {
                    foreach (var enumType in fileProto.EnumType)
                    {
                        foreach (var enumValue in enumType.Value)
                        {
                            var options = enumValue.Options;
                            if (options != null && options.HasExtension(PacketIdExtensions.HandlerName))
                            {
                                // ToString() 없이 식별자 자체를 전달
                                string handlerName = options.GetExtension(PacketIdExtensions.HandlerName);
                                string enumName = enumValue.Name; // 예: SERVICE_TYPE_LOGIN

                                // 추출한 데이터를 바탕으로 핸들러 초기화 문자열 구성
                                handleInitString += string.Format(
                                    PacketFormatter.SERVICE_TYPE_HANDLE_INIT_FORMAT,
                                    enumName,
                                    handlerName
                                );
                            }
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[GenerateInitHandleString] 패킷 핸들러 생성 중 오류 발생: {e.Message}");

                return false;
            }

            return true;
        }

        public static bool GenerateHandlerFile(eRole receiver, eRole sender, string protoDirPath, string outputDirPath)
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

                if (!GenerateInitHandleString(receiver, sender, $"{protoName}.proto", protoFilePath, out var initHandleString))
                {
                    return false;
                }

                if (!GenerateHandleFunctionDeclares(receiver, sender, $"{protoName}.proto", protoFilePath,
                        out var handleFunctionDeclareString))
                {
                    return false;
                }

                if (!GenerateMakeSendBufferFunction(receiver, sender, $"{protoName}.proto", protoFilePath,
                        out var makeSendBufferFunctionString))
                {
                    return false;
                }

                var handleFileContent = string.Format(PacketFormatter.HANDLE_FILE_FORMAT, protoName,
                    initHandleString,
                    handleFunctionDeclareString,
                    makeSendBufferFunctionString);

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

        private static bool GenerateInitHandleString(eRole receiver, eRole sender, string protoName, string filePath,
            out string initHandleString)
        {
            initHandleString = "";

            try
            {
                // 1. 레지스트리 생성 및 등록 (이 코드가 없으면 무조건 Unknown으로 빠집니다!)
                var registry = new ExtensionRegistry
                {
                    EnumExtensions.Sender,
                    EnumExtensions.Receiver
                };

                using var stream = File.OpenRead(filePath);
                var descriptorSet = FileDescriptorSet.Parser.WithExtensionRegistry(registry).ParseFrom(stream);
                foreach (var fileProto in descriptorSet.File)
                {
                    if (!fileProto.Name.EndsWith(protoName))
                    {
                        continue;
                    }

                    foreach (var msg in fileProto.MessageType)
                    {
                        var options = msg.Options;
                        if (options == null)
                        {
                            continue;
                        }

                        if (options.GetExtension(EnumExtensions.Receiver) != receiver && options.GetExtension(EnumExtensions.Sender) != sender)
                        {
                            continue;
                        }

                        var packetName = msg.Name;
                        initHandleString += string.Format(PacketFormatter.HANDLE_INIT_FORMAT, "ID_" + packetName,
                            packetName);
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[GenerateInitHandleString] 패킷 핸들러 생성 중 오류 발생: {e.Message}");

                return false;
            }

            return true;
        }

        private static bool GenerateHandleFunctionDeclares(eRole receiver, eRole sender, string protoName, string filePath,
            out string handleFunctionDeclareString)
        {
            handleFunctionDeclareString = "";

            try
            {
                // 1. 레지스트리 생성 및 등록 (이 코드가 없으면 무조건 Unknown으로 빠집니다!)
                var registry = new ExtensionRegistry
                {
                    EnumExtensions.Sender,
                    EnumExtensions.Receiver
                };

                using var stream = File.OpenRead(filePath);
                var descriptorSet = FileDescriptorSet.Parser.WithExtensionRegistry(registry).ParseFrom(stream);

                foreach (var fileProto in descriptorSet.File)
                {
                    if (!fileProto.Name.EndsWith(protoName))
                    {
                        continue;
                    }

                    foreach (var msg in fileProto.MessageType)
                    {
                        var options = msg.Options;
                        if (options.GetExtension(EnumExtensions.Receiver) != receiver && options.GetExtension(EnumExtensions.Sender) != sender)
                        {
                            continue;
                        }

                        var packetName = msg.Name;
                        handleFunctionDeclareString += string.Format(PacketFormatter.HANDLE_DECLARE_FORMAT, packetName);
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[GenerateHandleFunctionDeclares] 패킷 핸들러 생성 중 오류 발생: {e.Message}");

                return false;
            }

            return true;
        }

        private static bool GenerateMakeSendBufferFunction(eRole receiver, eRole sender, string protoName, string filePath,
            out string makeSendBufferFunctionString)
        {
            makeSendBufferFunctionString = "";
            try
            {
                // 1. 레지스트리 생성 및 등록 (이 코드가 없으면 무조건 Unknown으로 빠집니다!)
                var registry = new ExtensionRegistry
                {
                    EnumExtensions.Sender,
                    EnumExtensions.Receiver
                };

                using var stream = File.OpenRead(filePath);
                var descriptorSet = FileDescriptorSet.Parser.WithExtensionRegistry(registry).ParseFrom(stream);
                foreach (var fileProto in descriptorSet.File)
                {
                    if (!fileProto.Name.EndsWith(protoName))
                    {
                        continue;
                    }

                    foreach (var msg in fileProto.MessageType)
                    {
                        var options = msg.Options;
                        if (options == null)
                        {
                            continue;
                        }

                        if (options.GetExtension(EnumExtensions.Receiver) != sender && options.GetExtension(EnumExtensions.Sender) != receiver)
                        {
                            continue;
                        }

                        var packetName = msg.Name;
                        makeSendBufferFunctionString += string.Format(PacketFormatter.MAKE_SEND_BUFFER_FORMAT,
                            packetName, $"ID_{packetName}");
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[GenerateMakeSendBufferFunction] 패킷 핸들러 생성 중 오류 발생: {e.Message}");
                return false;
            }

            return true;

        }

    }
}
