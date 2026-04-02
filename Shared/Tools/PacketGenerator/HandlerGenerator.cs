using System.Text;

namespace PacketGenerator
{
    public static class HandlerGenerator
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
                            packet.MessageName, // {1}
                            handler.ProtoFileName); // {2}
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
