#include "ApplicationFixture.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetType.h>
#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetFactory.h>

#include <CoreUtilities/ThreadConfig.h>

#include <atomic>
#include <thread>

using namespace Volt;

// A separate asset type/GUID from AssetManagerTests.cpp so the registrars don't collide.
VT_DECLARE_ASSET_TYPE(ConcurrencyTestAssetType, "{2D9C1F7E-6B14-4F0A-9C3D-7E5A1B0C44D2}"_guid);
VT_REGISTER_ASSET_TYPE(ConcurrencyTestAssetType);

namespace IntegrationTests
{
	class AssetManagerConcurrencyFixture : public ApplicationFixture
	{};

	class ConcTestAsset : public Asset
	{
	public:
		ConcTestAsset() = default;
		~ConcTestAsset() override = default;
		ConcTestAsset(uint32_t inValue)
			: testValue(inValue)
		{}

		static AssetType GetStaticType() { return AssetTypes::ConcurrencyTestAssetType; }
		AssetType GetType() const override { return GetStaticType(); }

		uint32_t testValue = 0;
	};

	VT_REGISTER_ASSET_FACTORY(AssetTypes::ConcurrencyTestAssetType, ConcTestAsset);

	TEST_F(AssetManagerConcurrencyFixture, AssetCacheConcurrentGetAndReleaseNoResurrection)
	{
		AssetHandle handle = Asset::Null();
		{
			AssetReference<ConcTestAsset> asset = g_assetManager->CreateAsset<ConcTestAsset>("CacheRace", 42);
			handle = asset->GetAssetHandle();
		}
		// The creating reference is gone; the asset is now unloaded and evicted from
		// the cache, so each GetAssetImmediately below recreates + republishes it.

		ASSERT_TRUE(g_assetManager->IsValidAssetHandle(handle));

		constexpr int ThreadCount = 8;
		constexpr int IterationsPerThread = 500;

		std::atomic<bool> start = false;
		Vector<std::thread> threads;

		for (int t = 0; t < ThreadCount; ++t)
		{
			threads.emplace_back([&]()
			{
				Threads::InitializeThreadConfig(false, false, false);

				while (!start.load(std::memory_order::acquire)) {}

				for (int i = 0; i < IterationsPerThread; ++i)
				{
					AssetReference<ConcTestAsset> ref = g_assetManager->GetAssetImmediately<ConcTestAsset>(handle);
					if (ref)
					{
						// Touch the asset, then let the reference drop. The last drop
						// across all threads races other threads still inside TryGet.
						volatile uint32_t value = ref->testValue;
						(void)value;
					}
				}
			});
		}

		start.store(true, std::memory_order::release);

		for (std::thread& thread : threads)
		{
			thread.join();
		}

		// Reaching here without an assert/crash means the cache get/release path is
		// safe under contention.
		EXPECT_TRUE(g_assetManager->IsValidAssetHandle(handle));
	}

	TEST_F(AssetManagerConcurrencyFixture, RemoveAssetWhileReferencedIsSafe)
	{
		AssetReference<ConcTestAsset> asset = g_assetManager->CreateAsset<ConcTestAsset>("RemoveWhileReferenced", 7);
		const AssetHandle handle = asset->GetAssetHandle();

		// Remove while we still hold a live reference: metadata is erased now.
		g_assetManager->RemoveAsset(handle);
		EXPECT_FALSE(g_assetManager->IsValidAssetHandle(handle));

		// Releasing the last reference currently dereferences the removed metadata
		// inside QueueAssetForDestruction -> crash.
		asset = nullptr;

		EXPECT_FALSE(g_assetManager->IsValidAssetHandle(handle));
	}

	TEST_F(AssetManagerConcurrencyFixture, RemoveAssetAfterReleaseInvalidatesHandle)
	{
		AssetHandle handle = Asset::Null();
		{
			AssetReference<ConcTestAsset> asset = g_assetManager->CreateAsset<ConcTestAsset>("RemoveAfterRelease", 11);
			handle = asset->GetAssetHandle();
			ASSERT_TRUE(g_assetManager->IsValidAssetHandle(handle));
		}

		// Metadata still exists for an unloaded, non-memory asset.
		ASSERT_TRUE(g_assetManager->IsValidAssetHandle(handle));

		g_assetManager->RemoveAsset(handle);
		EXPECT_FALSE(g_assetManager->IsValidAssetHandle(handle));
	}

	TEST_F(AssetManagerConcurrencyFixture, SecondGetReturnsSameCachedInstance)
	{
		{
			AssetReference<ConcTestAsset> first = g_assetManager->CreateMemoryAsset<ConcTestAsset>("SameInstance", 123);
			const AssetHandle handle = first->GetAssetHandle();

			AssetReference<ConcTestAsset> second;
			ASSERT_TRUE(g_assetManager->TryGetAssetIfLoaded<ConcTestAsset>(handle, second));
			ASSERT_TRUE(second.IsValid());

			// Same underlying asset instance.
			EXPECT_EQ(&(*first), &(*second));
			EXPECT_EQ(second->testValue, 123u);
			EXPECT_GE(first->GetRefCount(), 2);
		}
	}

	// TryGetAssetIfLoaded must not load/queue anything and must report false once
	// the asset's metadata has been removed.
	TEST_F(AssetManagerConcurrencyFixture, TryGetIfLoadedFalseAfterRemove)
	{
		AssetHandle handle = Asset::Null();
		{
			AssetReference<ConcTestAsset> asset = g_assetManager->CreateAsset<ConcTestAsset>("IfLoadedAfterRemove", 5);
			handle = asset->GetAssetHandle();

			AssetReference<ConcTestAsset> loaded;
			EXPECT_TRUE(g_assetManager->TryGetAssetIfLoaded<ConcTestAsset>(handle, loaded));
			EXPECT_TRUE(loaded.IsValid());
		}

		g_assetManager->RemoveAsset(handle);

		AssetReference<ConcTestAsset> afterRemove;
		EXPECT_FALSE(g_assetManager->TryGetAssetIfLoaded<ConcTestAsset>(handle, afterRemove));
		EXPECT_FALSE(afterRemove.IsValid());
	}
}
