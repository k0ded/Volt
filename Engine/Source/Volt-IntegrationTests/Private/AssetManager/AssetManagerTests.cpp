#include "ApplicationFixture.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetType.h>
#include <AssetSystem/Asset_New.h>
#include <AssetSystem/AssetFactory.h>
#include <AssetSystem/AssetLocks.h>

using namespace Volt;

VT_DECLARE_ASSET_TYPE(TestingAssetType, "{44E34EE9-45C4-4678-A491-74E4553AEA4F}"_guid);
VT_REGISTER_ASSET_TYPE(TestingAssetType);

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

	TEST_F(AssetManagerFixture, CreateMemoryAsset)
	{
		AssetHandle newAssetHandle = Asset::Null();
		{
			AssetReference<TestAsset> newAsset = g_assetManager->CreateMemoryAsset<TestAsset>("TestingAsset", 1001);
			EXPECT_NE(newAsset, nullptr);

			ScopedAssetReferenceLock assetLock{ newAsset };

			newAssetHandle = newAsset->GetAssetHandle();
			EXPECT_NE(newAssetHandle, Asset::Null());

			EXPECT_EQ(newAsset->testValue, 1001);

			ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(newAssetHandle);
			EXPECT_TRUE(assetMetadata->IsMemoryAsset());
			EXPECT_TRUE(assetMetadata->IsLoaded());
		}

		// At this point the asset should have been released/removed
		EXPECT_FALSE(g_assetManager->IsValidAssetHandle(newAssetHandle));
	}

	TEST_F(AssetManagerFixture, CreateAsset)
	{
		AssetHandle newAssetHandle = Asset::Null();
		{
			AssetReference<TestAsset> newAsset = g_assetManager->CreateAsset<TestAsset>("TestingAsset", 1001);
			EXPECT_NE(newAsset, nullptr);

			ScopedAssetReferenceLock assetLock{ newAsset };

			newAssetHandle = newAsset->GetAssetHandle();
			EXPECT_NE(newAssetHandle, Asset::Null());

			EXPECT_EQ(newAsset->testValue, 1001);

			ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(newAssetHandle);
			EXPECT_FALSE(assetMetadata->IsMemoryAsset());
			EXPECT_TRUE(assetMetadata->IsLoaded());
		}

		// At this point the asset should have been released, but the metadata still valid.
		EXPECT_TRUE(g_assetManager->IsValidAssetHandle(newAssetHandle));

		{
			ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(newAssetHandle);
			EXPECT_FALSE(assetMetadata->IsLoaded());
		}
	}

	VT_REGISTER_ASSET_FACTORY(AssetTypes::TestingAssetType, TestAsset);
}

