#include "ApplicationFixture.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetType.h>
#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetFactory.h>

using namespace Volt;

VT_DECLARE_ASSET_TYPE(TestingAssetType, "{44E34EE9-45C4-4678-A491-74E4553AEA4F}"_guid);
VT_REGISTER_ASSET_TYPE(TestingAssetType);

// A second, distinct type used to exercise AssetRegistryIteratorFilter::AddAssetType<T>().
VT_DECLARE_ASSET_TYPE(TestingAssetTypeB, "{7F3B2C91-8E45-4A67-9D12-3C5E8F0A1B6D}"_guid);
VT_REGISTER_ASSET_TYPE(TestingAssetTypeB);

namespace IntergrationTests
{
	class AssetManagerFixture : public ApplicationFixture
	{};

	class TestAsset : public Asset
	{
	public:
		TestAsset() = default;
		~TestAsset() override = default;
		TestAsset(uint32_t inValue)
			: testValue(inValue)
		{}

		static AssetType GetStaticType() { return AssetTypes::TestingAssetType; }
		AssetType GetType() const override { return GetStaticType(); }

		uint32_t testValue;
	};

	class TestAssetB : public Asset
	{
	public:
		TestAssetB() = default;
		~TestAssetB() override = default;

		static AssetType GetStaticType() { return AssetTypes::TestingAssetTypeB; }
		AssetType GetType() const override { return GetStaticType(); }
	};

	TEST_F(AssetManagerFixture, CreateAssetWithAssetHandle_UsesProvidedHandle)
	{
		const AssetHandle explicitHandle{ 0x1234'5678'9ABC'DEF0ULL };

		AssetReference<TestAsset> newAsset = g_assetManager->CreateAssetWithAssetHandle<TestAsset>("ExplicitHandleAsset", explicitHandle, 7);

		EXPECT_EQ(newAsset->GetAssetHandle(), explicitHandle);
		EXPECT_TRUE(g_assetManager->IsValidAssetHandle(explicitHandle));

		AssetMetadata metadataCopy = g_assetManager->GetAssetMetadataCopy(explicitHandle);
		EXPECT_EQ(metadataCopy.handle, explicitHandle);
		EXPECT_EQ(metadataCopy.type, AssetTypes::TestingAssetType);
		EXPECT_FALSE(metadataCopy.IsMemoryAsset());
	}

	TEST_F(AssetManagerFixture, CreateAnonymousAsset_IsValidAndLoadedButExcludedFromIteration)
	{
		AssetReference<TestAsset> anonymous = g_assetManager->CreateAnonymousAsset<TestAsset>("AnonAsset", 42);
		const AssetHandle handle = anonymous->GetAssetHandle();

		EXPECT_TRUE(g_assetManager->IsValidAssetHandle(handle));
		EXPECT_TRUE(g_assetManager->IsAssetLoaded(handle));

		ReadOnlyAssetMetadata metadata = g_assetManager->GetReadOnlyAssetMetadata(handle);
		ASSERT_TRUE(metadata.IsValid());
		EXPECT_TRUE(metadata->IsMemoryAsset());

		bool found = false;
		g_assetManager->IterateAssetRegistryWithFilter({}, [&found, handle](ReadOnlyAssetMetadata it)
		{
			if (it->handle == handle)
			{
				found = true;
			}
			return true;
		});

		EXPECT_FALSE(found);
	}

	TEST_F(AssetManagerFixture, CreateAssetTypeless_CreatesLoadedAssetOfCorrectType)
	{
		AssetReference<Asset> typelessAsset = g_assetManager->CreateAssetTypeless("TypelessAsset", AssetTypes::TestingAssetType);
		const AssetHandle handle = typelessAsset->GetAssetHandle();

		EXPECT_TRUE(g_assetManager->IsValidAssetHandle(handle));
		EXPECT_TRUE(g_assetManager->IsAssetLoaded(handle));
		EXPECT_EQ(typelessAsset->GetType(), AssetTypes::TestingAssetType);

		// Default-constructed through the typeless path, so the value should be zero-initialized.
		AssetReference<TestAsset> typed = typelessAsset.ConvertTo<TestAsset>();
		ASSERT_TRUE(typed.IsValid());
		EXPECT_EQ(typed->testValue, 0u);
		EXPECT_EQ(static_cast<Asset*>(&(*typed)), &(*typelessAsset));
	}

	TEST_F(AssetManagerFixture, GetAssetImmediately_RefetchesSameCachedInstanceBeforeGarbageCollection)
	{
		AssetHandle handle = Asset::Null();
		TestAsset* originalPtr = nullptr;
		{
			AssetReference<TestAsset> asset = g_assetManager->CreateAsset<TestAsset>("RefetchAsset", 321);
			handle = asset->GetAssetHandle();
			originalPtr = &(*asset);
		}

		// No Tick() yet: the cache still holds the asset, so it is re-fetched rather than reloaded.
		AssetReference<TestAsset> refetched;
		ASSERT_TRUE(g_assetManager->TryGetAssetImmediately<TestAsset>(handle, refetched));
		EXPECT_EQ(&(*refetched), originalPtr);
		EXPECT_EQ(refetched->testValue, 321u);
		EXPECT_GE(refetched->GetRefCount(), 1);
	}

	TEST_F(AssetManagerFixture, TryGetAsset_OnAlreadyLoadedAsset_ReturnsSameInstance)
	{
		AssetReference<TestAsset> asset = g_assetManager->CreateMemoryAsset<TestAsset>("TryGetAsset", 5);
		const AssetHandle handle = asset->GetAssetHandle();

		AssetReference<TestAsset> fetched;
		EXPECT_TRUE(g_assetManager->TryGetAsset<TestAsset>(handle, fetched));
		ASSERT_TRUE(fetched.IsValid());
		EXPECT_EQ(fetched->testValue, 5u);
		EXPECT_EQ(&(*fetched), &(*asset));
	}

	TEST_F(AssetManagerFixture, TryGetTypelessAsset_And_TryGetTypelessAssetIfLoaded_ResolveToSameAsset)
	{
		AssetReference<TestAsset> asset = g_assetManager->CreateAsset<TestAsset>("TypelessGet", 11);
		const AssetHandle handle = asset->GetAssetHandle();

		AssetReference<Asset> typelessIfLoaded;
		EXPECT_TRUE(g_assetManager->TryGetTypelessAssetIfLoaded(handle, typelessIfLoaded));
		ASSERT_TRUE(typelessIfLoaded.IsValid());
		EXPECT_EQ(typelessIfLoaded->GetAssetHandle(), handle);

		AssetReference<Asset> typelessGet;
		EXPECT_TRUE(g_assetManager->TryGetTypelessAsset(handle, typelessGet));
		ASSERT_TRUE(typelessGet.IsValid());
		EXPECT_EQ(typelessGet->GetAssetHandle(), handle);
	}

	TEST_F(AssetManagerFixture, AssetMetadataAccessors_ReflectCreationAndAreIndependentCopies)
	{
		AssetReference<TestAsset> asset = g_assetManager->CreateAsset<TestAsset>("MetadataAsset", 55);
		const AssetHandle handle = asset->GetAssetHandle();

		{
			ReadOnlyAssetMetadata metadata = g_assetManager->GetReadOnlyAssetMetadata(handle);
			ASSERT_TRUE(metadata.IsValid());
			EXPECT_EQ(metadata->type, AssetTypes::TestingAssetType);
			EXPECT_FALSE(metadata->IsMemoryAsset());
			EXPECT_TRUE(metadata->IsLoaded());
			EXPECT_EQ(metadata->GetLoadState(), AssetLoadState::Loaded);
			EXPECT_FALSE(metadata->HasFilepath());
			EXPECT_FALSE(metadata->isEngineAsset);
		}

		{
			WriteableAssetMetadata metadata = g_assetManager->GetWriteableAssetMetadata(handle);
			ASSERT_TRUE(metadata.IsValid());
			metadata->isEngineAsset = true;
		}

		{
			ReadOnlyAssetMetadata metadata = g_assetManager->GetReadOnlyAssetMetadata(handle);
			EXPECT_TRUE(metadata->isEngineAsset);
		}

		AssetMetadata copy = g_assetManager->GetAssetMetadataCopy(handle);
		EXPECT_EQ(copy.handle, handle);
		EXPECT_TRUE(copy.isEngineAsset);

		// The copy must be independent of the live metadata.
		copy.isEngineAsset = false;
		ReadOnlyAssetMetadata stillTrue = g_assetManager->GetReadOnlyAssetMetadata(handle);
		EXPECT_TRUE(stillTrue->isEngineAsset);
	}

	TEST_F(AssetManagerFixture, AssetMetadataAccessors_InvalidHandlesReturnInvalidWrappers)
	{
		EXPECT_FALSE(g_assetManager->IsValidAssetHandle(Asset::Null()));

		ReadOnlyAssetMetadata nullReadOnly = g_assetManager->GetReadOnlyAssetMetadata(Asset::Null());
		EXPECT_FALSE(nullReadOnly.IsValid());

		WriteableAssetMetadata nullWriteable = g_assetManager->GetWriteableAssetMetadata(Asset::Null());
		EXPECT_FALSE(nullWriteable.IsValid());

		// A handle that was never registered, but isn't the null sentinel.
		const AssetHandle neverRegistered{ 0xBADDCAFEULL };
		EXPECT_FALSE(g_assetManager->IsValidAssetHandle(neverRegistered));

		AssetMetadata copy = g_assetManager->GetAssetMetadataCopy(neverRegistered);
		EXPECT_FALSE(copy.IsValid());
	}

	TEST_F(AssetManagerFixture, AssetChangedCallbacks_FireOnLoadAndUnload_RespectUnregister)
	{
		Vector<AssetChangedState> states;
		AssetHandle observedHandle = Asset::Null();

		AssetManager::AssetUpdatedCallbackID callbackId = g_assetManager->RegisterAssetUpdatedCallback(
			AssetTypes::TestingAssetType,
			[&states, &observedHandle](AssetHandle handle, AssetChangedState state)
			{
				observedHandle = handle;
				states.push_back(state);
			});

		AssetHandle handle = Asset::Null();
		{
			AssetReference<TestAsset> asset = g_assetManager->CreateAsset<TestAsset>("CallbackAsset", 1);
			handle = asset->GetAssetHandle();
		}

		// Drive garbage collection so the deferred eviction is actually processed and the
		// queued Loaded/Unloaded events are dispatched.
		Tick();

		ASSERT_EQ(states.size(), 2u);
		EXPECT_EQ(states[0], AssetChangedState::Loaded);
		EXPECT_EQ(states[1], AssetChangedState::Unloaded);
		EXPECT_EQ(observedHandle, handle);

		g_assetManager->UnregisterAssetUpdatedCallback(AssetTypes::TestingAssetType, callbackId);

		{
			AssetReference<TestAsset> secondAsset = g_assetManager->CreateAsset<TestAsset>("CallbackAsset2", 2);
		}

		Tick();

		// No new events should have been recorded after unregistering.
		EXPECT_EQ(states.size(), 2u);
	}

	TEST_F(AssetManagerFixture, IterateAssetRegistryWithFilter_DefaultIncludesEverythingButAnonymous)
	{
		AssetReference<TestAsset> regular = g_assetManager->CreateAsset<TestAsset>("FilterRegular", 1);
		AssetReference<TestAsset> memory = g_assetManager->CreateMemoryAsset<TestAsset>("FilterMemory", 2);
		AssetReference<TestAsset> anonymous = g_assetManager->CreateAnonymousAsset<TestAsset>("FilterAnonymous", 3);

		Vector<AssetHandle> visited;
		g_assetManager->IterateAssetRegistryWithFilter({}, [&visited](ReadOnlyAssetMetadata metadata)
		{
			visited.push_back(metadata->handle);
			return true;
		});

		auto Contains = [&visited](AssetHandle handle)
		{
			for (AssetHandle visitedHandle : visited)
			{
				if (visitedHandle == handle)
				{
					return true;
				}
			}
			return false;
		};

		EXPECT_TRUE(Contains(regular->GetAssetHandle()));
		EXPECT_TRUE(Contains(memory->GetAssetHandle()));
		EXPECT_FALSE(Contains(anonymous->GetAssetHandle()));
	}

	TEST_F(AssetManagerFixture, IterateAssetRegistryWithFilter_ExcludeMemoryAssets)
	{
		AssetReference<TestAsset> regular = g_assetManager->CreateAsset<TestAsset>("FilterRegular2", 1);
		AssetReference<TestAsset> memory = g_assetManager->CreateMemoryAsset<TestAsset>("FilterMemory2", 2);

		AssetRegistryIteratorFilter filter{};
		filter.includeMemoryAssets = false;

		Vector<AssetHandle> visited;
		g_assetManager->IterateAssetRegistryWithFilter(filter, [&visited](ReadOnlyAssetMetadata metadata)
		{
			visited.push_back(metadata->handle);
			return true;
		});

		auto Contains = [&visited](AssetHandle handle)
		{
			for (AssetHandle visitedHandle : visited)
			{
				if (visitedHandle == handle)
				{
					return true;
				}
			}
			return false;
		};

		EXPECT_TRUE(Contains(regular->GetAssetHandle()));
		EXPECT_FALSE(Contains(memory->GetAssetHandle()));
	}

	TEST_F(AssetManagerFixture, IterateAssetRegistryWithFilter_ExcludeWithoutFilepath_MemoryAssetsBypassFilter)
	{
		AssetReference<TestAsset> regularNoFilepath = g_assetManager->CreateAsset<TestAsset>("FilterRegular3", 1);
		AssetReference<TestAsset> memory = g_assetManager->CreateMemoryAsset<TestAsset>("FilterMemory3", 2);

		AssetRegistryIteratorFilter filter{};
		filter.includeWithoutFilepath = false;

		Vector<AssetHandle> visited;
		g_assetManager->IterateAssetRegistryWithFilter(filter, [&visited](ReadOnlyAssetMetadata metadata)
		{
			visited.push_back(metadata->handle);
			return true;
		});

		auto Contains = [&visited](AssetHandle handle)
		{
			for (AssetHandle visitedHandle : visited)
			{
				if (visitedHandle == handle)
				{
					return true;
				}
			}
			return false;
		};

		// Non-memory assets without a filepath are excluded...
		EXPECT_FALSE(Contains(regularNoFilepath->GetAssetHandle()));
		// ...but memory assets bypass this filter entirely, regardless of filepath state.
		EXPECT_TRUE(Contains(memory->GetAssetHandle()));
	}

	TEST_F(AssetManagerFixture, IterateAssetRegistryWithFilter_FilteredAssetTypesRestrictsToRequestedType)
	{
		AssetReference<TestAsset> typeA = g_assetManager->CreateAsset<TestAsset>("FilterTypeA", 1);
		AssetReference<TestAssetB> typeB = g_assetManager->CreateAsset<TestAssetB>("FilterTypeB");

		AssetRegistryIteratorFilter filter{};
		filter.AddAssetType<TestAsset>();

		Vector<AssetHandle> visited;
		g_assetManager->IterateAssetRegistryWithFilter(filter, [&visited](ReadOnlyAssetMetadata metadata)
		{
			visited.push_back(metadata->handle);
			return true;
		});

		auto Contains = [&visited](AssetHandle handle)
		{
			for (AssetHandle visitedHandle : visited)
			{
				if (visitedHandle == handle)
				{
					return true;
				}
			}
			return false;
		};

		EXPECT_TRUE(Contains(typeA->GetAssetHandle()));
		EXPECT_FALSE(Contains(typeB->GetAssetHandle()));
	}

	TEST_F(AssetManagerFixture, IterateAssetRegistryWithFilter_EarlyExitStopsIteration)
	{
		AssetReference<TestAsset> first = g_assetManager->CreateAsset<TestAsset>("EarlyExitFirst", 1);
		AssetReference<TestAsset> second = g_assetManager->CreateAsset<TestAsset>("EarlyExitSecond", 2);

		int32_t visitCount = 0;
		g_assetManager->IterateAssetRegistryWithFilter({}, [&visitCount](ReadOnlyAssetMetadata)
		{
			++visitCount;
			return false;
		});

		EXPECT_EQ(visitCount, 1);
	}

	TEST_F(AssetManagerFixture, IsEngineAsset_MatchesEngineAndEditorOnFirstPathSegment)
	{
		EXPECT_TRUE(g_assetManager->IsEngineAsset(Filesystem::Path("Engine/Some/Path.vtasset")));
		EXPECT_TRUE(g_assetManager->IsEngineAsset(Filesystem::Path("Editor/Foo.vtasset")));
		EXPECT_FALSE(g_assetManager->IsEngineAsset(Filesystem::Path("Project/Assets/Foo.vtasset")));
		EXPECT_FALSE(g_assetManager->IsEngineAsset(Filesystem::Path("Foo.vtasset")));
	}

	TEST_F(AssetManagerFixture, GetAssetFilesystemPath_AbsolutePathIsReturnedUnchanged)
	{
		const Filesystem::Path absolutePath("C:/SomeAbsoluteFolder/Test.vtasset");
		ASSERT_TRUE(absolutePath.IsAbsolute());

		EXPECT_EQ(g_assetManager->GetAssetFilesystemPath(absolutePath), absolutePath);
	}

	TEST_F(AssetManagerFixture, GetAssetHandleFromFilepath_ReturnsNullForUnknownPath)
	{
		EXPECT_EQ(g_assetManager->GetAssetHandleFromFilepath(Filesystem::Path("Some/Unknown/Path.vtasset")), Asset::Null());
	}

	TEST_F(AssetManagerFixture, AssetReference_CopyMoveSemanticsAdjustRefCount)
	{
		AssetReference<TestAsset> a = g_assetManager->CreateMemoryAsset<TestAsset>("RefCountAsset", 7);
		TestAsset* rawPtr = &(*a);
		EXPECT_EQ(a->GetRefCount(), 2); // construction ref + cache token

		{
			AssetReference<TestAsset> b = a; // copy
			EXPECT_EQ(a->GetRefCount(), 3);
			EXPECT_EQ(&(*b), rawPtr);
		}

		EXPECT_EQ(a->GetRefCount(), 2);

		AssetReference<TestAsset> c = std::move(a);
		EXPECT_EQ(c->GetRefCount(), 2);
		EXPECT_FALSE(a.IsValid());
		EXPECT_EQ(&(*c), rawPtr);
	}

	TEST_F(AssetManagerFixture, AssetReference_ConvertToUpcastAndDowncastIncrementRefCount)
	{
		AssetReference<TestAsset> typed = g_assetManager->CreateMemoryAsset<TestAsset>("ConvertAsset", 99);
		EXPECT_EQ(typed->GetRefCount(), 2);

		AssetReference<Asset> upcast = typed; // implicit upcast via the converting constructor
		EXPECT_EQ(typed->GetRefCount(), 3);
		EXPECT_EQ(&(*upcast), static_cast<Asset*>(&(*typed)));

		AssetReference<TestAsset> downcast = upcast.ConvertTo<TestAsset>();
		EXPECT_EQ(typed->GetRefCount(), 4);
		EXPECT_EQ(downcast->testValue, 99u);
		EXPECT_EQ(&(*downcast), &(*typed));
	}

	TEST_F(AssetManagerFixture, SaveAsset_EarlyReturnGuardsDoNotCrashOrMutateState)
	{
		AssetReference<TestAsset> memoryAsset = g_assetManager->CreateMemoryAsset<TestAsset>("SaveMemoryGuard", 1);
		g_assetManager->SaveAsset(memoryAsset->GetAssetHandle());
		EXPECT_TRUE(g_assetManager->IsValidAssetHandle(memoryAsset->GetAssetHandle()));

		AssetReference<TestAsset> noFilepathAsset = g_assetManager->CreateAsset<TestAsset>("SaveNoFilepathGuard", 2);
		g_assetManager->SaveAsset(noFilepathAsset->GetAssetHandle());
		EXPECT_TRUE(g_assetManager->IsValidAssetHandle(noFilepathAsset->GetAssetHandle()));

		// A handle that was never registered.
		g_assetManager->SaveAsset(AssetHandle{ 0xDEADBEEFULL });
	}

	TEST_F(AssetManagerFixture, GetNumAssetsInRegistry_IncreasesWhenAssetsAreCreated)
	{
		EXPECT_NE(g_assetManager->GetMetadataLoadingCounter(), nullptr);

		const int32_t before = g_assetManager->GetNumAssetsInRegistry();

		AssetReference<TestAsset> first = g_assetManager->CreateAsset<TestAsset>("CountAsset1", 1);
		AssetReference<TestAsset> second = g_assetManager->CreateMemoryAsset<TestAsset>("CountAsset2", 2);
		AssetReference<TestAsset> third = g_assetManager->CreateAnonymousAsset<TestAsset>("CountAsset3", 3);

		// Background metadata loading from disk may still be inserting entries concurrently,
		// so only assert a lower bound rather than exact equality.
		EXPECT_GE(g_assetManager->GetNumAssetsInRegistry(), before + 3);
	}

	TEST_F(AssetManagerFixture, GetAssetsDependentOn_CurrentlyAlwaysReturnsEmpty)
	{
		AssetReference<TestAsset> asset = g_assetManager->CreateAsset<TestAsset>("DependentOnAsset", 1);
		EXPECT_TRUE(g_assetManager->GetAssetsDependentOn(asset->GetAssetHandle()).empty());
	}

	TEST_F(AssetManagerFixture, RemoveAsset_InvalidatesHandleImmediatelyEvenWhileReferenced)
	{
		AssetReference<TestAsset> asset = g_assetManager->CreateAsset<TestAsset>("RemoveMeAsset", 1);
		const AssetHandle handle = asset->GetAssetHandle();
		ASSERT_TRUE(g_assetManager->IsValidAssetHandle(handle));

		g_assetManager->RemoveAsset(handle);

		EXPECT_FALSE(g_assetManager->IsValidAssetHandle(handle));

		ReadOnlyAssetMetadata metadata = g_assetManager->GetReadOnlyAssetMetadata(handle);
		EXPECT_FALSE(metadata.IsValid());
	}

	VT_REGISTER_ASSET_FACTORY(AssetTypes::TestingAssetType, TestAsset);
	VT_REGISTER_ASSET_FACTORY(AssetTypes::TestingAssetTypeB, TestAssetB);
}

