#pragma once

namespace packet_id
{
	enum eEchoPacketId : uint16
	{
        C2S_ECHO_REQ = 1,
        S2C_ECHO_RES = 2,
	};


	enum eLoginPacketId : uint16
	{
        C2S_LOGIN_REQ = 1,
        S2C_LOGIN_RES = 2,
	};

}