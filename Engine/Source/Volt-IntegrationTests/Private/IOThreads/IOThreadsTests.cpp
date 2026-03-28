#include "ApplicationFixture.h"

#include <Volt-FileSystem/FileUtility.h>
#include <Volt-FileSystem/Filesystem.h>

#include <CoreUtilities/Containers/Array.h>

#include <thread>
#include <atomic>
#include <barrier>

using namespace Volt;

namespace IntegrationTests
{
	class IOThreadsFixture : public ApplicationFixture
	{
	protected:
		void SetUp() override
		{
			ApplicationFixture::SetUp();

			m_testDirectory = Filesystem::Path("__io_threads_test_tmp");
			Filesystem::CreateDirectories(m_testDirectory);
		}

		void TearDown() override
		{
			Filesystem::RemoveAll(m_testDirectory);
		}

		Filesystem::Path CreateTestFile(StringView name, String&& content)
		{
			Filesystem::Path filepath = m_testDirectory / Filesystem::Path(name);
			FileUtility::WriteStringToFile(filepath, std::move(content));
			return filepath;
		}

		Filesystem::Path m_testDirectory;
	};

	TEST_F(IOThreadsFixture, ReadStringFromFile_SingleRead)
	{
		const Filesystem::Path filepath = CreateTestFile("single_read.txt", String("Hello, World!"));

		String result;
		const bool success = FileUtility::ReadStringFromFile(filepath, result);

		EXPECT_TRUE(success);
		EXPECT_EQ(result, "Hello, World!");
	}

	TEST_F(IOThreadsFixture, ReadStringFromFile_SequentialHighContention)
	{
		const Filesystem::Path filepath = CreateTestFile("sequential_contention.txt", String("TestContent"));

		constexpr uint32_t NumIterations = 2048;

		for (uint32_t i = 0; i < NumIterations; ++i)
		{
			String result;
			const bool success = FileUtility::ReadStringFromFile(filepath, result);

			ASSERT_TRUE(success) << "Failed on iteration " << i;
			ASSERT_EQ(result, "TestContent") << "Mismatched content on iteration " << i;
		}
	}

	TEST_F(IOThreadsFixture, ReadStringFromFile_ConcurrentReadsFromMultipleThreads)
	{
		const Filesystem::Path filepath = CreateTestFile("concurrent_reads.txt", String("ConcurrentData"));

		constexpr uint32_t NumThreads = 16;
		constexpr uint32_t NumReadsPerThread = 256;

		std::atomic_uint32_t failureCount = 0;
		std::atomic_uint32_t contentMismatchCount = 0;
		std::barrier syncPoint(NumThreads);

		Array<std::thread, NumThreads> threads;

		for (uint32_t t = 0; t < NumThreads; ++t)
		{
			threads[t] = std::thread([&]()
			{
				syncPoint.arrive_and_wait();

				for (uint32_t i = 0; i < NumReadsPerThread; ++i)
				{
					String result;
					const bool success = FileUtility::ReadStringFromFile(filepath, result);

					if (!success)
					{
						failureCount.fetch_add(1, std::memory_order::relaxed);
					}
					else if (result != "ConcurrentData")
					{
						contentMismatchCount.fetch_add(1, std::memory_order::relaxed);
					}
				}
			});
		}

		for (auto& thread : threads)
		{
			thread.join();
		}

		EXPECT_EQ(failureCount.load(), 0u) << "Some reads failed under contention";
		EXPECT_EQ(contentMismatchCount.load(), 0u) << "Some reads returned incorrect content";
	}

	TEST_F(IOThreadsFixture, ReadStringFromFile_ConcurrentReadsMultipleFiles)
	{
		constexpr uint32_t NumFiles = 8;
		constexpr uint32_t NumThreads = 16;
		constexpr uint32_t NumReadsPerThread = 128;

		Array<Filesystem::Path, NumFiles> filepaths;
		Array<String, NumFiles> expectedContents;

		for (uint32_t f = 0; f < NumFiles; ++f)
		{
			expectedContents[f] = FormatString("FileContent_{}", f);
			filepaths[f] = CreateTestFile(
				FormatString("multi_file_{}.txt", f),
				String(expectedContents[f]));
		}

		std::atomic_uint32_t failureCount = 0;
		std::atomic_uint32_t contentMismatchCount = 0;
		std::barrier syncPoint(NumThreads);

		Array<std::thread, NumThreads> threads;

		for (uint32_t t = 0; t < NumThreads; ++t)
		{
			threads[t] = std::thread([&, t]()
			{
				syncPoint.arrive_and_wait();

				for (uint32_t i = 0; i < NumReadsPerThread; ++i)
				{
					const uint32_t fileIndex = (t + i) % NumFiles;

					String result;
					const bool success = FileUtility::ReadStringFromFile(filepaths[fileIndex], result);

					if (!success)
					{
						failureCount.fetch_add(1, std::memory_order::relaxed);
					}
					else if (result != expectedContents[fileIndex])
					{
						contentMismatchCount.fetch_add(1, std::memory_order::relaxed);
					}
				}
			});
		}

		for (auto& thread : threads)
		{
			thread.join();
		}

		EXPECT_EQ(failureCount.load(), 0u) << "Some reads failed under contention";
		EXPECT_EQ(contentMismatchCount.load(), 0u) << "Some reads returned incorrect content";
	}

	TEST_F(IOThreadsFixture, ReadStringFromFile_ConcurrentReadWrite)
	{
		const Filesystem::Path filepath = CreateTestFile("read_write_contention.txt", String("InitialContent"));

		constexpr uint32_t NumReaderThreads = 12;
		constexpr uint32_t NumWriterThreads = 4;
		constexpr uint32_t NumOpsPerThread = 128;

		std::atomic_bool running = true;
		std::atomic_uint32_t readFailures = 0;
		std::atomic_uint32_t writeFailures = 0;
		std::barrier syncPoint(NumReaderThreads + NumWriterThreads);

		Array<std::thread, NumReaderThreads> readerThreads;
		Array<std::thread, NumWriterThreads> writerThreads;

		for (uint32_t t = 0; t < NumReaderThreads; ++t)
		{
			readerThreads[t] = std::thread([&]()
			{
				syncPoint.arrive_and_wait();

				for (uint32_t i = 0; i < NumOpsPerThread; ++i)
				{
					String result;
					const bool success = FileUtility::ReadStringFromFile(filepath, result);

					if (!success)
					{
						readFailures.fetch_add(1, std::memory_order::relaxed);
					}
				}
			});
		}

		for (uint32_t t = 0; t < NumWriterThreads; ++t)
		{
			writerThreads[t] = std::thread([&, t]()
			{
				syncPoint.arrive_and_wait();

				for (uint32_t i = 0; i < NumOpsPerThread; ++i)
				{
					String content = FormatString("WriterContent_{}_{}", t, i);
					FileUtility::WriteStringToFile(filepath, std::move(content));
				}
			});
		}

		for (auto& thread : readerThreads)
		{
			thread.join();
		}

		for (auto& thread : writerThreads)
		{
			thread.join();
		}

		EXPECT_EQ(readFailures.load(), 0u) << "Reads failed during concurrent read/write";
	}

	TEST_F(IOThreadsFixture, ReadStringFromFile_RapidFireRequestsStressTest)
	{
		constexpr uint32_t NumFiles = 4;
		constexpr uint32_t NumThreads = 16;
		constexpr uint32_t NumReadsPerThread = 512;

		Array<Filesystem::Path, NumFiles> filepaths;

		for (uint32_t f = 0; f < NumFiles; ++f)
		{
			filepaths[f] = CreateTestFile(
				FormatString("stress_{}.txt", f),
				FormatString("StressData_{}", f));
		}

		std::atomic_uint32_t completedReads = 0;
		std::atomic_uint32_t failures = 0;
		std::barrier syncPoint(NumThreads);

		Array<std::thread, NumThreads> threads;

		for (uint32_t t = 0; t < NumThreads; ++t)
		{
			threads[t] = std::thread([&, t]()
			{
				syncPoint.arrive_and_wait();

				for (uint32_t i = 0; i < NumReadsPerThread; ++i)
				{
					const uint32_t fileIndex = (t * i + i) % NumFiles;

					String result;
					const bool success = FileUtility::ReadStringFromFile(filepaths[fileIndex], result);

					if (success)
					{
						completedReads.fetch_add(1, std::memory_order::relaxed);
					}
					else
					{
						failures.fetch_add(1, std::memory_order::relaxed);
					}
				}
			});
		}

		for (auto& thread : threads)
		{
			thread.join();
		}

		const uint32_t totalExpected = NumThreads * NumReadsPerThread;
		EXPECT_EQ(completedReads.load() + failures.load(), totalExpected);
		EXPECT_EQ(failures.load(), 0u) << "Some IO requests failed under stress";
	}
}
