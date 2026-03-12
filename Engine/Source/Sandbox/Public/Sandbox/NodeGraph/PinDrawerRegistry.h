#pragma once

#include <NodeGraph/NodeTypeBase.h>


class PinDrawerRegistry
{
public:
	struct PinDrawerInfo
	{
		VoltGUID pinTypeGUID;
		std::function<void(void*/*storagePtr*/, const void* /*customDataPtr*/)> drawFn;
	};

	static class PinDrawerRegistry& Get();

	template<typename DrawerType>
	void RegisterDrawer()
	{
		VT_ENSURE(!HasDrawerForType(DrawerType::PinType::GetStaticTypeGUID()));

		PinDrawerInfo drawerInfo;
		drawerInfo.pinTypeGUID = DrawerType::PinType::GetStaticTypeGUID();
		drawerInfo.drawFn = [](void* storagePtr, const void* customDataPtr)
		{
			DrawerType::DrawImGui(
				*reinterpret_cast<DrawerType::PinType::StorageType*>(storagePtr),
				*reinterpret_cast<const DrawerType::PinType::CustomDataType*>(customDataPtr));
		};

		m_pinDrawers.insert({ drawerInfo.pinTypeGUID, std::move(drawerInfo) });
	}
	template<typename DrawerType>
	void UnregisterDrawer()
	{
		m_pinDrawers.erase(DrawerType::PinType::GetStaticTypeGUID());
	}

	bool HasDrawerForType(VoltGUID pinTypeGuid)
	{
		return m_pinDrawers.contains(pinTypeGuid);
	}

	void DrawPinType(VoltGUID pinTypeGuid, void* storagePtr, const void* customDataPtr)
	{
		VT_ENSURE(HasDrawerForType(pinTypeGuid));
		m_pinDrawers[pinTypeGuid].drawFn(storagePtr, customDataPtr);
	}

private:
	std::unordered_map<VoltGUID, PinDrawerInfo> m_pinDrawers;
};

// Must lie in a compilation unit (cpp file)
#define REGISTER_PIN_DRAWER_FOR_TYPE(targetPinType)																\
class PinDrawer_##targetPinType																					\
{																												\
public:																											\
	typedef targetPinType PinType;																				\
	PinDrawer_##targetPinType()																					\
	{																											\
		PinDrawerRegistry::Get().RegisterDrawer<PinDrawer_##targetPinType>();									\
	}																											\
	~PinDrawer_##targetPinType()																				\
	{																											\
		PinDrawerRegistry::Get().UnregisterDrawer<PinDrawer_##targetPinType>();									\
	}																											\
	static void DrawImGui(targetPinType::StorageType& storage, const targetPinType::CustomDataType& customData);\
} g_pinDrawerRegistrar_##targetPinType;																			\
void PinDrawer_##targetPinType::DrawImGui(targetPinType::StorageType& storage, const targetPinType::CustomDataType& customData)
