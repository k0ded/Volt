#pragma once

namespace Circuit
{
	enum class ReplyResult
	{
		Handled,
		Unhandled,
	};
	class Reply
	{
	public:
		static Reply Unhandled();
		static Reply Handled();


		ReplyResult GetResult();
	private:
		ReplyResult m_replyResult;
	};
}
