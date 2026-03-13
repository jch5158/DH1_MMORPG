#include "pch.h"
#include "Message.h"

Message::Message(MessageType&& message)
	:mMessage(std::move(message))
{
}

void Message::Execute() const
{
	if (mMessage != nullptr)
	{
		mMessage();
	}
}

