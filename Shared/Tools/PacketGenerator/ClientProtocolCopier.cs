namespace PacketGenerator
{
    /// <summary>
    /// Copies generated .pb.h / .pb.cpp into the UE client module tree (same sources as Shared/Protocol, single compile unit).
    /// </summary>
    internal static class ClientProtocolCopier
    {
        private static readonly HashSet<string> BaseProtoFiles = new()
        {
            "Enum", "Struct", "PacketOption"
        };

        public static void CopyClientProtocolHeaders(
            string protoSourceDir,
            string clientProtocolDir,
            List<HandlerInfo> handlers,
            string clientRole)
        {
            if (string.IsNullOrEmpty(clientProtocolDir))
            {
                return;
            }

            if (!Directory.Exists(clientProtocolDir))
            {
                Directory.CreateDirectory(clientProtocolDir);
            }

            var usedProtoFiles = new HashSet<string>(BaseProtoFiles);

            foreach (var handler in handlers)
            {
                foreach (var packet in handler.Packets)
                {
                    if (packet.Receivers.Any(r => RoleHelper.IsMatchRole(r, clientRole)) ||
                        RoleHelper.IsMatchRole(packet.Sender, clientRole))
                    {
                        usedProtoFiles.Add(handler.ProtoFileName);
                    }
                }
            }

            foreach (var file in Directory.GetFiles(clientProtocolDir, "*.pb.h"))
            {
                File.Delete(file);
            }

            foreach (var file in Directory.GetFiles(clientProtocolDir, "*.pb.cpp"))
            {
                File.Delete(file);
            }

            foreach (var file in Directory.GetFiles(clientProtocolDir, "*.pb.cc"))
            {
                File.Delete(file);
            }

            var copiedSources = 0;
            foreach (var protoFile in usedProtoFiles)
            {
                var sourceHeader = Path.Combine(protoSourceDir, $"{protoFile}.pb.h");
                if (File.Exists(sourceHeader))
                {
                    File.Copy(sourceHeader, Path.Combine(clientProtocolDir, $"{protoFile}.pb.h"), true);
                }

                // .pb.cc → .pb.cpp : UBT scans module .cpp; keep one definition (in client module, not a separate .lib)
                var sourceCc = Path.Combine(protoSourceDir, $"{protoFile}.pb.cc");
                if (File.Exists(sourceCc))
                {
                    File.Copy(sourceCc, Path.Combine(clientProtocolDir, $"{protoFile}.pb.cpp"), true);
                    copiedSources++;
                }
            }

            Console.WriteLine($"[ClientProtocol] Copied {usedProtoFiles.Count} headers and {copiedSources} generated sources to: {clientProtocolDir}");
        }
    }
}
