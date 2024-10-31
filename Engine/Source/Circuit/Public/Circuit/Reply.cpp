#include "circuitpch.h"
#include "Reply.h"

namespace Circuit
{
	Reply Circuit::Reply::Unhandled()
	{
		Reply result;
		result.m_replyResult = ReplyResult::Unhandled;
		return result;
	}

	Reply Circuit::Reply::Handled()
	{
		Reply result;
		result.m_replyResult = ReplyResult::Handled;
		return result;
	}

	ReplyResult Reply::GetResult()
	{
		return m_replyResult;
	}
}
