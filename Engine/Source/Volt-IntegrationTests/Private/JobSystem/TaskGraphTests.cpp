#include "ApplicationFixture.h"

#include <JobSystem/TaskGraph.h>

#include <atomic>
#include <mutex>
#include <vector>

using namespace Volt;

namespace IntegrationTests
{
    class TaskGraphFixture : public ApplicationFixture
    { };

    // Verifies that a single task with no dependencies executes and completes.
    TEST_F(TaskGraphFixture, SingleTask_Executes)
    {
        std::atomic_bool ran = false;

        TaskGraph graph(ExecutionPriority::Critical);
        graph.AddTask("Single", [&ran]() { ran.store(true, std::memory_order_release); });
        graph.ExecuteAndWait();

        EXPECT_TRUE(ran.load(std::memory_order_acquire));
    }

    // Verifies that all independent tasks (no dependencies) all complete.
    TEST_F(TaskGraphFixture, MultipleTasks_NoDependencies_AllExecute)
    {
        constexpr uint32_t NumTasks = 8;
        std::atomic_uint32_t runCount = 0;

        TaskGraph graph(ExecutionPriority::Critical);
        for (uint32_t i = 0; i < NumTasks; ++i)
        {
            graph.AddTask("Independent", [&runCount]() { runCount.fetch_add(1, std::memory_order_relaxed); });
        }
        graph.ExecuteAndWait();

        EXPECT_EQ(runCount.load(), NumTasks);
    }

    // Verifies that a dependency runs before the task that depends on it.
    TEST_F(TaskGraphFixture, LinearChain_DependencyRunsFirst)
    {
        std::atomic_int32_t order = 0;
        int32_t depFinishedAt  = -1;
        int32_t taskFinishedAt = -1;

        TaskGraph graph(ExecutionPriority::Critical);

        TaskGraph::Task* dep = graph.AddTask("Dependency",
            [&]()
            {
                depFinishedAt = order.fetch_add(1, std::memory_order_acq_rel);
            });

        graph.AddTaskWithDependencies("Dependant", { dep },
            [&]()
            {
                taskFinishedAt = order.fetch_add(1, std::memory_order_acq_rel);
            });

        graph.ExecuteAndWait();

        EXPECT_EQ(depFinishedAt, 0);
        EXPECT_EQ(taskFinishedAt, 1);
    }

    // Verifies a three-step linear chain executes in the correct order: C -> B -> A.
    TEST_F(TaskGraphFixture, LinearChain_ThreeTasks_CorrectOrder)
    {
        std::atomic_int32_t order = 0;
        int32_t orderC = -1;
        int32_t orderB = -1;
        int32_t orderA = -1;

        TaskGraph graph(ExecutionPriority::Critical);

        TaskGraph::Task* taskC = graph.AddTask("C",
            [&]() { orderC = order.fetch_add(1, std::memory_order_acq_rel); });

        TaskGraph::Task* taskB = graph.AddTaskWithDependencies("B", { taskC },
            [&]() { orderB = order.fetch_add(1, std::memory_order_acq_rel); });

        graph.AddTaskWithDependencies("A", { taskB },
            [&]() { orderA = order.fetch_add(1, std::memory_order_acq_rel); });

        graph.ExecuteAndWait();

        EXPECT_EQ(orderC, 0);
        EXPECT_EQ(orderB, 1);
        EXPECT_EQ(orderA, 2);
    }

    // The core regression test: a shared dependency (C) must run exactly once
    // even though both A and B depend on it, and both A and B must still run.
    TEST_F(TaskGraphFixture, SharedDependency_DependencyRunsExactlyOnce)
    {
        std::atomic_uint32_t sharedRunCount = 0;
        std::atomic_bool ranA = false;
        std::atomic_bool ranB = false;

        TaskGraph graph(ExecutionPriority::Critical);

        TaskGraph::Task* shared = graph.AddTask("Shared",
            [&]() { sharedRunCount.fetch_add(1, std::memory_order_relaxed); });

        graph.AddTaskWithDependencies("A", { shared },
            [&]() { ranA.store(true, std::memory_order_release); });

        graph.AddTaskWithDependencies("B", { shared },
            [&]() { ranB.store(true, std::memory_order_release); });

        graph.ExecuteAndWait();

        EXPECT_EQ(sharedRunCount.load(), 1u) << "Shared dependency must run exactly once, not once per dependant.";
        EXPECT_TRUE(ranA.load(std::memory_order_acquire));
        EXPECT_TRUE(ranB.load(std::memory_order_acquire));
    }

    // Verifies that when A and B both depend on C, neither A nor B runs before C completes.
    TEST_F(TaskGraphFixture, SharedDependency_DependantsRunAfterShared)
    {
        std::atomic_bool sharedDone = false;
        std::atomic_bool orderViolated = false;

        TaskGraph graph(ExecutionPriority::Critical);

        TaskGraph::Task* shared = graph.AddTask("Shared",
            [&]() { sharedDone.store(true, std::memory_order_release); });

        graph.AddTaskWithDependencies("A", { shared },
            [&]()
            {
                if (!sharedDone.load(std::memory_order_acquire))
                    orderViolated.store(true, std::memory_order_relaxed);
            });

        graph.AddTaskWithDependencies("B", { shared },
            [&]()
            {
                if (!sharedDone.load(std::memory_order_acquire))
                    orderViolated.store(true, std::memory_order_relaxed);
            });

        graph.ExecuteAndWait();

        EXPECT_FALSE(orderViolated.load());
    }

    // Diamond pattern: A depends on B and C, both B and C depend on D.
    //   D must run first, then B and C (in any order), then A last.
    TEST_F(TaskGraphFixture, DiamondDependency_CorrectOrdering)
    {
        std::atomic_bool dDone  = false;
        std::atomic_bool bRan   = false;
        std::atomic_bool cRan   = false;
        std::atomic_bool orderViolated = false;
        std::atomic_uint32_t dRunCount = 0;

        TaskGraph graph(ExecutionPriority::Critical);

        // D: the base, shared by both B and C.
        TaskGraph::Task* taskD = graph.AddTask("D",
            [&]()
            {
                dRunCount.fetch_add(1, std::memory_order_relaxed);
                dDone.store(true, std::memory_order_release);
            });

        // B depends on D.
        TaskGraph::Task* taskB = graph.AddTaskWithDependencies("B", { taskD },
            [&]()
            {
                if (!dDone.load(std::memory_order_acquire)) orderViolated.store(true, std::memory_order_relaxed);
                bRan.store(true, std::memory_order_release);
            });

        // C depends on D.
        TaskGraph::Task* taskC = graph.AddTaskWithDependencies("C", { taskD },
            [&]()
            {
                if (!dDone.load(std::memory_order_acquire)) orderViolated.store(true, std::memory_order_relaxed);
                cRan.store(true, std::memory_order_release);
            });

        // A depends on both B and C.
        graph.AddTaskWithDependencies("A", { taskB, taskC },
            [&]()
            {
                if (!bRan.load(std::memory_order_acquire)) orderViolated.store(true, std::memory_order_relaxed);
                if (!cRan.load(std::memory_order_acquire)) orderViolated.store(true, std::memory_order_relaxed);
            });

        graph.ExecuteAndWait();

        EXPECT_EQ(dRunCount.load(), 1u) << "Shared base task D must run exactly once.";
        EXPECT_TRUE(bRan.load(std::memory_order_acquire));
        EXPECT_TRUE(cRan.load(std::memory_order_acquire));
        EXPECT_FALSE(orderViolated.load());
    }

    // Verifies that multiple independent tasks sharing one dependency all run
    // and that the shared dependency ran exactly once.
    TEST_F(TaskGraphFixture, MultipleTasksOneSharedDependency_AllRun)
    {
        constexpr uint32_t NumDependants = 16;

        std::atomic_uint32_t sharedRunCount  = 0;
        std::atomic_uint32_t dependantRunCount = 0;

        TaskGraph graph(ExecutionPriority::Critical);

        TaskGraph::Task* shared = graph.AddTask("Shared",
            [&]() { sharedRunCount.fetch_add(1, std::memory_order_relaxed); });

        for (uint32_t i = 0; i < NumDependants; ++i)
        {
            graph.AddTaskWithDependencies("Dependant", { shared },
                [&]() { dependantRunCount.fetch_add(1, std::memory_order_relaxed); });
        }

        graph.ExecuteAndWait();

        EXPECT_EQ(sharedRunCount.load(), 1u);
        EXPECT_EQ(dependantRunCount.load(), NumDependants);
    }

    // Verifies that an empty graph does not crash or deadlock.
    TEST_F(TaskGraphFixture, EmptyGraph_DoesNotCrash)
    {
        TaskGraph graph(ExecutionPriority::Critical);
        graph.ExecuteAndWait();
    }

    // Verifies that tasks added via a span of dependencies all execute correctly.
    TEST_F(TaskGraphFixture, SpanDependencies_AllExecute)
    {
        std::atomic_uint32_t depRunCount = 0;
        std::atomic_bool rootRan = false;

        TaskGraph graph(ExecutionPriority::Critical);

        Vector<TaskGraph::Task*> deps;
        for (uint32_t i = 0; i < 4; ++i)
        {
            deps.push_back(graph.AddTask("Dep", [&]() { depRunCount.fetch_add(1, std::memory_order_relaxed); }));
        }

        graph.AddTaskWithDependencies("Root", std::span<TaskGraph::Task*>(deps.data(), deps.size()),
            [&]() { rootRan.store(true, std::memory_order_release); });

        graph.ExecuteAndWait();

        EXPECT_EQ(depRunCount.load(), 4u);
        EXPECT_TRUE(rootRan.load(std::memory_order_acquire));
    }
}
