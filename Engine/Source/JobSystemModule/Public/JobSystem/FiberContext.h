#pragma once

#include <emmintrin.h>

namespace Volt
{
	struct FiberContext
	{
		void* rip, * rsp;
		void* rbx, * rbp, * r12, * r13, * r14, * r15, * rdi, * rsi;

		alignas(16) __m128i xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;

		// TEB
		void* stackBase;
		void* stackLimit;
		void* exceptionList;
		void* deallocationStack;

		// SSE & x87 FPU
		uint32_t mxcsr;
		uint32_t fpucw;

		void* userdata;
	};

	struct FiberThreadFPState
	{
		uint32_t mxcsr;
		uint32_t fpucw;
	};
}
