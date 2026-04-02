namespace PacketGenerator
{
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
}
