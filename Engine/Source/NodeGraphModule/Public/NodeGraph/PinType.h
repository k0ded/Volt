#pragma once

#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/Containers/VectorVariants.h>

//struct PinTypeInfo
//{
//	VoltGUID guid;
//
//
//};
//
//class NodeGraphPinTypeRegistry
//{
//public:
//	static NodeGraphPinTypeRegistry& Get();
//
//	template<typename T> 
//	void RegisterPinType()
//	{
//		const VoltGUID guid = T::GetStaticGUID();
//
//		PinTypeInfo typeInfo;
//		m_pinTypes.insert(guid, typeInfo);
//	}
//	template<typename T> 
//	void UnregisterPinType()
//	{
//	}
//	
//private:
//	std::unordered_map<VoltGUID, PinTypeInfo> m_pinTypes;
//};
constexpr uint8_t PIN_TYPE_CUSTOM_DATA_MAX_SIZE = 32;
typedef InlineVector<uint8_t, PIN_TYPE_CUSTOM_DATA_MAX_SIZE> PinTypeCustomDataVector;



#define DECLARE_PIN_TYPE_CUSTOM_DATA_EXPORT(pinTypeName, customDataType, guid, api)		\
struct api PinType_##pinTypeName														\
{																						\
	typedef customDataType CustomDataType;												\
	static VoltGUID GetStaticGUID() { return guid; }									\
																						\
	static_assert(sizeof(CustomDataType) <= PIN_TYPE_CUSTOM_DATA_MAX_SIZE);				\
}

#define DECLARE_PIN_TYPE_CUSTOM_DATA(pinTypeName, customDataType, guid)	DECLARE_PIN_TYPE_CUSTOM_DATA_EXPORT(pinTypeName, customDataType, guid, )

struct NoPinCustomData {};
#define DECLARE_PIN_TYPE_EXPORT(pinTypeName, guid, api) DECLARE_PIN_TYPE_CUSTOM_DATA_EXPORT(pinTypeName, NoPinCustomData, guid, api)
#define DECLARE_PIN_TYPE(pinTypeName, guid) DECLARE_PIN_TYPE_EXPORT(pinTypeName, guid, )


//#define REGISTER_PIN_TYPE(pinTypeName) PinType_##pinTypeName g_pinTypeRegistrar_##pinTypeName
