#pragma once

#include "CoreUtilities/VoltAssert.h"

#include <thread>

class ThreadAccessValidator
{
public:
	ThreadAccessValidator() = default;
	~ThreadAccessValidator() = default;

private:
	friend class ThreadAccessGuard;

	std::thread::id m_activeThread;
};


class ThreadAccessGuard
{
public:
	ThreadAccessGuard(ThreadAccessValidator& validator)
		: m_validator(validator)
	{
		VT_ENSURE_MSG(m_validator.m_activeThread._Get_underlying_id() == 0, "Validator is accessed by more then one thread! This is invalid!");
		m_validator.m_activeThread = std::this_thread::get_id();
	}

	~ThreadAccessGuard()
	{
		m_validator.m_activeThread = {};
	}

private:
	ThreadAccessValidator& m_validator;
};
