#include "espch.h"
#include "EntitySystem/Scripting/ECSEnvironmentStorage.h"
#include "EntitySystem/Scripting/ECSSystemRegistry.h"

ECSEnvironmentStorage::~ECSEnvironmentStorage()
{
	Clear();
}

void ECSEnvironmentStorage::InitializeWith(const Vector<ECSEnvironmentDefinition>& environmentDefinitions)
{
	// Make sure all previous held data is released properly
	Clear();

	// Find the total environment storage size
	// to be able to allocate the correct buffer size
	size_t totalEnvironmentStorageSize = 0;
	for (const auto& definition : environmentDefinitions)
	{
		totalEnvironmentStorageSize += definition.typeSize;
	}

	m_dataBuffer.Allocate(totalEnvironmentStorageSize);

	// Allocate each environment type and store a pointer to it.
	for (size_t currentOffset = 0; const auto& definition : environmentDefinitions)
	{
		void* currentDataPtr = m_dataBuffer.As<void>(currentOffset);
		definition.construct(currentDataPtr);
		currentOffset += definition.typeSize;

		m_environmentTypeIndexToEnvData[definition.typeIndex] = { currentDataPtr, definition };
	}
}

void ECSEnvironmentStorage::Clear()
{
	for (const auto& [typeIndex, data] : m_environmentTypeIndexToEnvData)
	{
		data.definition.destruct(data.dataPtr);
	}

	m_dataBuffer.Clear();
	m_environmentTypeIndexToEnvData.clear();
}
