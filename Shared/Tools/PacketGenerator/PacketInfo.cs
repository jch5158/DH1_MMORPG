namespace PacketGenerator
{
    public class PacketInfo
    {
        public string MessageName { get; set; } = string.Empty;
        public uint PacketId { get; set; }
        public string Sender { get; set; } = string.Empty;
        public List<string> Receivers { get; set; } = [];
    }

    public class HandlerInfo
    {
        public string ProtoFileName { get; set; } = string.Empty;
        public string HandlerName { get; set; } = string.Empty;
        public string ServiceTypeName { get; set; } = string.Empty;
        public List<PacketInfo> Packets { get; set; } = [];
    }
}
