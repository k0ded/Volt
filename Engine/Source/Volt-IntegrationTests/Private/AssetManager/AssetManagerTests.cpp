#include "ApplicationFixture.h"

#include <AssetSystem/AssetManager_New.h>
#include <AssetSystem/AssetType.h>
#include <AssetSystem/Asset_New.h>
#include <AssetSystem/AssetFactory.h>

using namespace Volt;

VT_DECLARE_ASSET_TYPE(TestingAssetType, "{44E34EE9-45C4-4678-A491-74E4553AEA4F}"_guid);
VT_REGISTER_ASSET_TYPE(TestingAssetType);

namespace IntergrationTests
{
	class AssetManagerFixture : public ApplicationFixture
	{};

	class TestAsset : public Asset_New
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
		RefPtr<TestAsset> newAsset = g_assetManager->CreateMemoryAsset<TestAsset>("TestingAsset", 1001);
		EXPECT_NE(newAsset, nullptr);
		EXPECT_EQ(newAsset->testValue, 1001);
	}

	VT_REGISTER_ASSET_FACTORY(AssetTypes::TestingAssetType, TestAsset);
}

