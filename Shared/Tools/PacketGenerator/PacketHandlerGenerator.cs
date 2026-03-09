using Google.Protobuf.Reflection;
using Google.Protobuf;

namespace PacketGenerator
{
    internal class PacketHandlerGenerator
    {
        public static bool Generate(PacketProjectsConfig config, string protoPath, string prjBasePath)
        {
            Path.Combine(protoPath + "Enum.desc");

            // 1. protoc가 생성한 .desc 파일 로드
            byte[] descBytes = File.ReadAllBytes(Path.Combine(protoPath + @"/Enum.desc")); // 예: "Protocol.desc"
            var descriptorSet = FileDescriptorSet.Parser.ParseFrom(descBytes);

            // 2. .desc 파일 내부에서 "eRole" 이라는 이름의 Enum 구조체 찾기
            var eRoleDescriptor = descriptorSet.File
                .SelectMany(f => f.EnumType)
                .FirstOrDefault(e => e.Name == "eRole");

            if (eRoleDescriptor == null)
            {
                Console.WriteLine("[Error] .desc 파일에서 'eRole' Enum을 찾을 수 없습니다.");
                return false;
            }

            // 3. eRole에 정의된 값(문자열)들을 HashSet으로 추출 (예: "Client", "Server")
            var validRoles = eRoleDescriptor.Value.Select(v => v.Name).ToHashSet(StringComparer.OrdinalIgnoreCase);

            foreach (var projectReceiver in config.Projects)
            {
                foreach (var projectSender in config.Projects)
                {
                    // 1. 여기서 양쪽 모두 대문자로 변환하여 변수에 담음
                    string receiverRole = projectReceiver.Role.ToUpper();
                    string senderRole = projectSender.Role.ToUpper();

                    // Enum.TryParse 대신, .desc에서 뽑아온 문자열 풀(Pool)에 존재하는지 검증
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

                    // 주의: 이제 eRole 이라는 C# 타입은 없으므로, Generate 함수의 매개변수를 string으로 변경해야 합니다.
                    if (!GenerateHandlerFile(receiverRole, senderRole, protoPath, outputPath))
                    {
                        Console.WriteLine("GenerateHandlerFile is Failed");
                        return false;
                    }

                    if (!GenerateServiceTypeHandlerFile(receiverRole, senderRole, protoPath, outputPath))
                    {
                        Console.WriteLine("GenerateServiceTypeHandlerFile is Failed");
                        return false;
                    }
                }
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
                using var stream = File.OpenRead(filePath);
                var descriptorSet = FileDescriptorSet.Parser.ParseFrom(stream);

                // 1. "handler_name" 확장의 태그 번호를 동적으로 찾기 (proto 파일에 정의된 이름에 맞게 매칭)
                // 보통 소문자 스네이크 케이스(handler_name)로 정의되므로 소문자로 비교합니다.
                var handlerNameExt = descriptorSet.File
                    .SelectMany(f => f.Extension)
                    .FirstOrDefault(e => e.Name.ToLower() == "handler_name");

                if (handlerNameExt == null)
                {
                    Console.WriteLine("[Error] .desc에서 handler_name(또는 HandlerName) 확장을 찾을 수 없습니다.");
                    return false;
                }

                int handlerNameFieldNum = handlerNameExt.Number;

                // 2. 전체 파일의 Enum을 순회
                foreach (var fileProto in descriptorSet.File)
                {
                    foreach (var enumType in fileProto.EnumType)
                    {
                        foreach (var enumValue in enumType.Value)
                        {
                            if (enumValue.Options == null) continue;

                            // 3. EnumValueOptions 바이너리를 직접 읽어서 C# 클래스 없이 값 추출
                            byte[] optionsBytes = enumValue.Options.ToByteArray();
                            var input = new CodedInputStream(optionsBytes);

                            string? handlerName = null;

                            while (!input.IsAtEnd)
                            {
                                uint tag = input.ReadTag();
                                int fieldNum = WireFormat.GetTagFieldNumber(tag);

                                if (fieldNum == handlerNameFieldNum)
                                {
                                    // 팩트: handler_name은 문자열이므로 ReadString()을 사용
                                    handlerName = input.ReadString();
                                }
                                else
                                {
                                    input.SkipLastField(); // 필요 없는 옵션은 스킵
                                }
                            }

                            // 4. handlerName을 성공적으로 파싱했다면 포맷에 맞춰 문자열 조립
                            if (!string.IsNullOrEmpty(handlerName))
                            {
                                string enumName = enumValue.Name; // 예: SERVICE_TYPE_LOGIN
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
                Console.WriteLine($"[GenerateHandleInitString] 패킷 핸들러 생성 중 오류 발생: {e.Message}");
                return false;
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

                    handleFileContent = handleFileContent.Replace("\r\n", "\n").Replace("\n", "\r\n");

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

        private static bool GenerateInitHandleString(string receiver, string sender, string protoName, string filePath,
            out string initHandleString)
        {
            initHandleString = "";

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

                int senderFieldNum = senderExt.Number;
                int receiverFieldNum = receiverExt.Number;

                // 4. 메시지 탐색
                foreach (var fileProto in descriptorSet.File)
                {
                    if (!fileProto.Name.EndsWith(protoName)) continue;

                    foreach (var msg in fileProto.MessageType)
                    {
                        if (msg.Options == null)
                        {
                            continue;
                        }

                        // 5. 핵심: MessageOptions 바이너리를 직접 읽어서 C# 클래스 없이 값 추출
                        byte[] optionsBytes = msg.Options.ToByteArray();
                        var input = new CodedInputStream(optionsBytes);

                        int currentSenderVal = -1;
                        int currentReceiverVal = -1;

                        while (!input.IsAtEnd)
                        {
                            uint tag = input.ReadTag();
                            int fieldNum = WireFormat.GetTagFieldNumber(tag);

                            if (fieldNum == senderFieldNum)
                            {
                                currentSenderVal = input.ReadEnum(); // Enum은 내부적으로 정수(Varint)임
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

                        // 6. 추출된 정수를 대문자 문자열로 변환하여 파라미터(receiver, sender)와 일치하는지 검증
                        if (currentSenderVal != -1 && currentReceiverVal != -1)
                        {
                            if (roleMap.TryGetValue(currentSenderVal, out string? sName) &&
                                roleMap.TryGetValue(currentReceiverVal, out string? rName))
                            {
                                if (sName == sender && rName == receiver) // 둘 다 이미 대문자이므로 완벽 매칭
                                {
                                    var packetName = msg.Name;
                                    initHandleString += string.Format(PacketFormatter.HANDLE_INIT_FORMAT, "ID_" + packetName, packetName);
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[GenerateInitHandleString] 오류 발생: {e.Message}");
                return false;
            }

            return true;
        }

        private static bool GenerateHandleFunctionDeclares(string receiver, string sender, string protoName, string filePath,
            out string handleFunctionDeclareString)
        {
            handleFunctionDeclareString = "";

            try
            {
                // 1. 레지스트리 의존성 완전 제거: 순수 .desc 파일만 파싱
                using var stream = File.OpenRead(filePath);
                var descriptorSet = FileDescriptorSet.Parser.ParseFrom(stream);

                // 2. 동적으로 eRole 매핑 딕셔너리 생성 (정수 -> 대문자 문자열)
                var eRoleDescriptor = descriptorSet.File
                    .SelectMany(f => f.EnumType)
                    .FirstOrDefault(e => e.Name == "eRole");

                if (eRoleDescriptor == null) return false;
                var roleMap = eRoleDescriptor.Value.ToDictionary(v => v.Number, v => v.Name.ToUpper());

                // 3. 동적으로 sender와 receiver의 확장 필드 번호 찾기
                var senderExt = descriptorSet.File.SelectMany(f => f.Extension).FirstOrDefault(e => e.Name == "sender");
                var receiverExt = descriptorSet.File.SelectMany(f => f.Extension).FirstOrDefault(e => e.Name == "receiver");

                if (senderExt == null || receiverExt == null)
                {
                    Console.WriteLine("[Error] .desc에서 sender 또는 receiver 확장을 찾을 수 없습니다.");
                    return false;
                }

                int senderFieldNum = senderExt.Number;
                int receiverFieldNum = receiverExt.Number;

                // 4. 메시지 탐색
                foreach (var fileProto in descriptorSet.File)
                {
                    if (!fileProto.Name.EndsWith(protoName)) continue;

                    foreach (var msg in fileProto.MessageType)
                    {
                        if (msg.Options == null) continue;

                        // 5. 핵심: MessageOptions 바이너리를 직접 읽어서 C# 클래스 없이 값 추출
                        byte[] optionsBytes = msg.Options.ToByteArray();
                        var input = new CodedInputStream(optionsBytes);

                        int currentSenderVal = -1;
                        int currentReceiverVal = -1;

                        while (!input.IsAtEnd)
                        {
                            uint tag = input.ReadTag();
                            int fieldNum = WireFormat.GetTagFieldNumber(tag);

                            if (fieldNum == senderFieldNum)
                            {
                                currentSenderVal = input.ReadEnum(); // Enum은 내부적으로 정수(Varint)
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

                        // 6. 추출된 정수를 대문자 문자열로 변환하여 파라미터(receiver, sender)와 일치하는지 검증
                        if (currentSenderVal != -1 && currentReceiverVal != -1)
                        {
                            if (roleMap.TryGetValue(currentSenderVal, out string? sName) &&
                                roleMap.TryGetValue(currentReceiverVal, out string? rName))
                            {
                                // 파라미터로 넘어온 sender/receiver는 이미 대문자이므로 안전하게 매칭됨
                                if (sName == sender && rName == receiver)
                                {
                                    var packetName = msg.Name;
                                    // 팩트: 여기만 HANDLE_DECLARE_FORMAT으로 함수 선언부를 조립합니다.
                                    handleFunctionDeclareString += string.Format(PacketFormatter.HANDLE_DECLARE_FORMAT, packetName);
                                }
                            }
                        }
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

        private static bool GenerateMakeSendBufferFunction(string receiver, string sender, string protoName, string filePath,
            out string makeSendBufferFunctionString)
        {
            // (함수 시그니처가 생략되어 있으니 내부 로직만 교체하십시오)
            makeSendBufferFunctionString = "";

            try
            {
                // 1. 레지스트리 의존성 완전 제거: 순수 .desc 파일만 파싱
                using var stream = File.OpenRead(filePath);
                var descriptorSet = FileDescriptorSet.Parser.ParseFrom(stream);

                // 2. 동적으로 eRole 매핑 딕셔너리 생성 (정수 -> 대문자 문자열)
                var eRoleDescriptor = descriptorSet.File
                    .SelectMany(f => f.EnumType)
                    .FirstOrDefault(e => e.Name == "eRole");

                if (eRoleDescriptor == null) return false;
                var roleMap = eRoleDescriptor.Value.ToDictionary(v => v.Number, v => v.Name.ToUpper());

                // 3. 동적으로 sender와 receiver의 확장 필드 번호 찾기
                var senderExt = descriptorSet.File.SelectMany(f => f.Extension).FirstOrDefault(e => e.Name == "sender");
                var receiverExt = descriptorSet.File.SelectMany(f => f.Extension).FirstOrDefault(e => e.Name == "receiver");

                if (senderExt == null || receiverExt == null)
                {
                    Console.WriteLine("[Error] .desc에서 sender 또는 receiver 확장을 찾을 수 없습니다.");
                    return false;
                }

                int senderFieldNum = senderExt.Number;
                int receiverFieldNum = receiverExt.Number;

                // 4. 메시지 탐색
                foreach (var fileProto in descriptorSet.File)
                {
                    if (!fileProto.Name.EndsWith(protoName)) continue;

                    foreach (var msg in fileProto.MessageType)
                    {
                        if (msg.Options == null) continue;

                        // 5. 핵심: MessageOptions 바이너리를 직접 읽어서 C# 클래스 없이 값 추출
                        byte[] optionsBytes = msg.Options.ToByteArray();
                        var input = new CodedInputStream(optionsBytes);

                        int currentSenderVal = -1;
                        int currentReceiverVal = -1;

                        while (!input.IsAtEnd)
                        {
                            uint tag = input.ReadTag();
                            int fieldNum = WireFormat.GetTagFieldNumber(tag);

                            if (fieldNum == senderFieldNum)
                            {
                                currentSenderVal = input.ReadEnum(); // Enum은 내부적으로 정수(Varint)
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

                        // 6. 추출된 정수를 대문자 문자열로 변환하여 검증
                        if (currentSenderVal != -1 && currentReceiverVal != -1)
                        {
                            if (roleMap.TryGetValue(currentSenderVal, out string? sName) &&
                                roleMap.TryGetValue(currentReceiverVal, out string? rName))
                            {
                                // ⚠️ 팩트: 원본 코드의 의도대로 교차(Cross) 매칭을 수행합니다.
                                // 패킷의 수신자(rName) == 파라미터 sender && 패킷의 송신자(sName) == 파라미터 receiver
                                if (rName == sender && sName == receiver)
                                {
                                    var packetName = msg.Name;
                                    makeSendBufferFunctionString += string.Format(PacketFormatter.MAKE_SEND_BUFFER_FORMAT,
                                        packetName, $"ID_{packetName}");
                                }
                            }
                        }
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
