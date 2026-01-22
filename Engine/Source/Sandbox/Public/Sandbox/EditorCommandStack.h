#pragma once

#include <Volt-Scene/Components/CoreComponents.h>
#include <Volt-Scene/EntityDescSerialization.h>
#include <Volt-Scene/Scene.h>

#include <EntitySystem/Entity.h>

#include <AssetSystem/AssetManager.h>

#include <stack>
#include "EditorCommand.h"
#include "Sandbox/Utility/EditorUtilities.h"


#include <tuple>

class EditorCommandStack
{
public:
	static EditorCommandStack& GetInstance();
	static void PushUndo(Ref<EditorCommand> cmd, bool redoAction = false);
	static void PushRedo(Ref<EditorCommand> cmd);
	static void Undo();
	static void Redo();
	static void Clear();
	void Update(const int aMaxStackSize);

private:
	inline static Vector<Ref<EditorCommand>> myUndoStack;
	inline static Vector<Ref<EditorCommand>> myRedoStack;
};

template<typename T>
struct ValueCommand : EditorCommand
{
	ValueCommand(T* aValueAdress, T aPreviousValue, Volt::Scene& targetScene, Volt::EntityID targetEntityID)
		: myValueAdress(aValueAdress), myPreviousValue(aPreviousValue), m_targetScene(targetScene), m_targetEntityID(targetEntityID)
	{}
	void Execute() override {}

	void Undo() override
	{
		Ref<ValueCommand<T>> command = CreateRef<ValueCommand<T>>(myValueAdress, *myValueAdress, m_targetScene, m_targetEntityID);
		EditorCommandStack::GetInstance().PushRedo(command);
		*myValueAdress = myPreviousValue;

		Volt::Entity entity = m_targetScene.GetEntityFromID(m_targetEntityID);
		EditorUtils::MarkEntityAsEdited(m_targetScene, entity);
	}

	void Redo() override
	{
		Ref<ValueCommand<T>> command = CreateRef<ValueCommand<T>>(myValueAdress, *myValueAdress, m_targetScene, m_targetEntityID);
		EditorCommandStack::GetInstance().PushUndo(command, true);
		*myValueAdress = myPreviousValue;

		Volt::Entity entity = m_targetScene.GetEntityFromID(m_targetEntityID);
		EditorUtils::MarkEntityAsEdited(m_targetScene, entity);
	}

private:
	T* myValueAdress;
	const T myPreviousValue;

	Volt::EntityID m_targetEntityID;
	Volt::Scene& m_targetScene;
};

struct GizmoCommand : EditorCommand
{
	struct GizmoData
	{
		glm::vec3* positionAdress;
		glm::quat* rotationAdress;
		glm::vec3* scaleAdress;

		glm::vec3 previousPositionValue;
		glm::quat previousRotationValue;
		glm::vec3 previousScaleValue;

		AssetReference<Volt::Scene> scene;
		Volt::EntityID id;
	};

	GizmoCommand(GizmoData aGizmoData)
		: myPositionAdress(aGizmoData.positionAdress),
		myRotationAdress(aGizmoData.rotationAdress),
		myScaleAdress(aGizmoData.scaleAdress),
		myPreviousPositionValue(aGizmoData.previousPositionValue),
		myPreviousRotationValue(aGizmoData.previousRotationValue),
		myPreviousScaleValue(aGizmoData.previousScaleValue), myID(aGizmoData.id), myScene(aGizmoData.scene)
	{}
	void Execute() override {}

	void Undo() override
	{
		GizmoData data;
		data.positionAdress = myPositionAdress;
		data.rotationAdress = myRotationAdress;
		data.scaleAdress = myScaleAdress;
		data.previousPositionValue = *myPositionAdress;

		data.previousRotationValue = *myRotationAdress;
		data.previousScaleValue = *myScaleAdress;
		data.id = myID;
		data.scene = myScene;

		Ref<GizmoCommand> command = CreateRef<GizmoCommand>(data);
		EditorCommandStack::GetInstance().PushRedo(command);

		*myPositionAdress = myPreviousPositionValue;
		*myRotationAdress = myPreviousRotationValue;
		*myScaleAdress = myPreviousScaleValue;

		if (myScene)
		{
			myScene->InvalidateEntityTransform(myID);
			EditorUtils::MarkEntityAndChildrenComponentAsEdited(*myScene, myScene->GetEntityFromID(myID), Volt::GetTypeGUID<Volt::TransformComponent>());
		}
	}

	void Redo() override
	{
		GizmoData data;
		data.positionAdress = myPositionAdress;
		data.rotationAdress = myRotationAdress;
		data.scaleAdress = myScaleAdress;
		data.previousPositionValue = *myPositionAdress;
		data.previousRotationValue = *myRotationAdress;
		data.previousScaleValue = *myScaleAdress;
		data.id = myID;
		data.scene = myScene;

		Ref<GizmoCommand> command = CreateRef<GizmoCommand>(data);
		EditorCommandStack::GetInstance().PushUndo(command, true);

		*myPositionAdress = myPreviousPositionValue;
		*myRotationAdress = myPreviousRotationValue;
		*myScaleAdress = myPreviousScaleValue;

		if (myScene)
		{
			myScene->InvalidateEntityTransform(myID);
			EditorUtils::MarkEntityAndChildrenComponentAsEdited(*myScene, myScene->GetEntityFromID(myID), Volt::GetTypeGUID<Volt::TransformComponent>());
		}
	}

private:
	glm::vec3* myPositionAdress;
	glm::quat* myRotationAdress;
	glm::vec3* myScaleAdress;
	const glm::vec3 myPreviousPositionValue;
	const glm::quat myPreviousRotationValue;
	const glm::vec3 myPreviousScaleValue;

	Volt::EntityID myID;
	AssetReference<Volt::Scene> myScene;
};

struct MultiGizmoCommand : EditorCommand
{
	MultiGizmoCommand(AssetReference<Volt::Scene> scene, const Vector<std::pair<Volt::EntityID, Volt::TransformComponent>>& entities)
		: myPreviousTransforms(entities), myScene(scene)
	{}

	void Execute() override {}

	void Undo() override
	{
		if (!myScene)
		{
			return;
		}

		auto scenePtr = myScene;
		Vector<std::pair<Volt::EntityID, Volt::TransformComponent>> currentTransforms;

		for (const auto& [id, oldComp] : myPreviousTransforms)
		{
			Volt::Entity entity = scenePtr->GetEntityFromID(id);
			Volt::TransformComponent transformComponent = entity.GetComponent<Volt::TransformComponent>();

			currentTransforms.emplace_back(id, transformComponent);
			entity.GetComponent<Volt::TransformComponent>() = oldComp;

			scenePtr->InvalidateEntityTransform(entity.GetID());
			EditorUtils::MarkEntityAndChildrenComponentAsEdited(*myScene, myScene->GetEntityFromID(id), Volt::GetTypeGUID<Volt::TransformComponent>());
		}

		Ref<MultiGizmoCommand> command = CreateRef<MultiGizmoCommand>(myScene, currentTransforms);
		EditorCommandStack::GetInstance().PushRedo(command);
	}

	void Redo() override
	{
		if (!myScene)
		{
			return;
		}

		auto scenePtr = myScene;
		Vector<std::pair<Volt::EntityID, Volt::TransformComponent>> currentTransforms;

		for (const auto& [id, oldComp] : myPreviousTransforms)
		{
			Volt::Entity entity = scenePtr->GetEntityFromID(id);
			Volt::TransformComponent transformComponent = entity.GetComponent<Volt::TransformComponent>();

			currentTransforms.emplace_back(id, transformComponent);
			entity.GetComponent<Volt::TransformComponent>() = oldComp;

			scenePtr->InvalidateEntityTransform(entity.GetID());
			EditorUtils::MarkEntityAndChildrenComponentAsEdited(*myScene, myScene->GetEntityFromID(id), Volt::GetTypeGUID<Volt::TransformComponent>());
		}

		Ref<MultiGizmoCommand> command = CreateRef<MultiGizmoCommand>(myScene, currentTransforms);
		EditorCommandStack::GetInstance().PushUndo(command);
	}

private:
	Vector<std::pair<Volt::EntityID, Volt::TransformComponent>> myPreviousTransforms;
	AssetReference<Volt::Scene> myScene;
};

enum class ObjectStateAction
{
	Create,
	Delete
};

struct ObjectStateCommand : EditorCommand
{
	ObjectStateCommand(Vector<Volt::Entity> entityList, Volt::Scene& targetScene, ObjectStateAction action) :
		m_Action(action), m_targetScene(targetScene)
	{
		VT_ENSURE(entityList.size() > 0);
		VT_ENSURE(&m_targetScene.GetEntityScene() == entityList[0].GetSceneReference());

		m_entityIDs.reserve(entityList.size());
		for (Volt::Entity& entity : entityList)
		{
			m_entityIDs.push_back(entity.GetID());
		}

		//if the action is delete, we need to save the data so that we can recreate the actor later
		if (action == ObjectStateAction::Delete)
		{
			MakeSerializationData();
		}
	}

	ObjectStateCommand(Volt::Entity entity, Volt::Scene& targetScene, ObjectStateAction action) :
		m_Action(action), m_targetScene(targetScene)
	{
		VT_ENSURE(&m_targetScene.GetEntityScene() == entity.GetSceneReference());
		m_entityIDs.push_back(entity.GetID());

		//if the action is delete, we need to save the data so that we can recreate the actor later
		if (action == ObjectStateAction::Delete)
		{
			MakeSerializationData();
		}
	}

	ObjectStateCommand(Vector<Volt::EntityID> entityIDs, Volt::Scene& targetScene, ObjectStateAction action) :
		m_entityIDs(entityIDs), m_targetScene(targetScene), m_Action(action)
	{
		//if the action is delete, we need to save the data so that we can recreate the actor later
		if (m_Action == ObjectStateAction::Delete)
		{
			MakeSerializationData();
		}
	}

	void Execute() override
	{}

	void Undo() override
	{
		Perform(true);
	}

	void Redo() override
	{
		Perform(false);
	}

private:
	void Perform(bool undo)
	{
		if (m_Action == ObjectStateAction::Create)
		{
			Ref<ObjectStateCommand> command = CreateRef<ObjectStateCommand>(m_entityIDs, m_targetScene, ObjectStateAction::Delete);
			if (undo)
			{
				EditorCommandStack::PushRedo(command);
			}
			else
			{
				EditorCommandStack::PushUndo(command, true);
			}

			for (Volt::EntityID& id : m_entityIDs)
			{
				Volt::Entity entity = m_targetScene.GetEntityFromID(id);
				m_targetScene.DestroyEntity(entity, true);
				EditorUtils::MarkEntityAsEdited(m_targetScene, entity);
			}
		}
		else if (m_Action == ObjectStateAction::Delete)
		{
			CreateEntitiesFromData();

			Ref<ObjectStateCommand> command = CreateRef<ObjectStateCommand>(m_entityIDs, m_targetScene, ObjectStateAction::Create);
			if (undo)
			{
				EditorCommandStack::PushRedo(command);
			}
			else
			{
				EditorCommandStack::PushUndo(command, true);
			}
		}
	}

	void MakeSerializationData()
	{
		m_idToAssociatedEntityDesc.clear();
		m_idToSerializedData.clear();
		for (Volt::EntityID& entityID : m_entityIDs)
		{
			m_idToAssociatedEntityDesc.insert({ entityID, m_targetScene.GetEntityDescHandleFromEntityID(entityID) });
			Volt::Entity entity = m_targetScene.GetEntityFromID(entityID);

			MemoryWriter writer{};
			Volt::EntityDescSerialization::SerializeEntity(writer, entity, Volt::Asset::Null());
			writer.Close();

			m_idToSerializedData.insert({ entityID, writer });

		}
	}

	void CreateEntitiesFromData()
	{
		for (Volt::EntityID entityID : m_entityIDs)
		{
			Volt::AssetHandle entityDescHandle = m_idToAssociatedEntityDesc.at(entityID);

			MemoryWriter& writer = m_idToSerializedData.at(entityID);
			MemoryReader reader(writer.GetData(), writer.GetSize());

			Volt::Entity entity;
			if (g_assetManager->IsValidAssetHandle(entityDescHandle))
			{
				entity = m_targetScene.CreateEntityWithIDForExistingDescription(entityID, entityDescHandle);
			}
			else
			{
				entity = m_targetScene.CreateEntityWithID(entityID);
			}

			Volt::EntityDescSerialization::DeserializeEntity(reader, entity);

			//since we manually deserialize these entities in place, we have to initialize their components manually aswell
			entity.InitializeComponents();

			EditorUtils::MarkEntityAsEdited(m_targetScene, entity);
		}


		m_idToAssociatedEntityDesc.clear();
		m_idToSerializedData.clear();
	}

	Volt::Scene& m_targetScene;
	Vector<Volt::EntityID> m_entityIDs;
	std::unordered_map<Volt::EntityID, MemoryWriter> m_idToSerializedData;
	std::unordered_map<Volt::EntityID, Volt::AssetHandle> m_idToAssociatedEntityDesc;

	ObjectStateAction m_Action;
};

enum class ParentingAction
{
	Parent,
	Unparent
};

struct ParentChildData
{
	Volt::Entity myParent;
	Volt::Entity myChild;
};

struct ParentingCommand : EditorCommand
{
	ParentingCommand(Vector<Ref<ParentChildData>> aData, ParentingAction anAction, Volt::Scene& targetScene) :
		myData(aData), myAction(anAction), m_targetScene(targetScene)
	{}

	void Execute() override
	{}

	void Undo() override
	{
		if (myAction == ParentingAction::Parent)
		{
			Ref<ParentingCommand> command = CreateRef<ParentingCommand>(myData, ParentingAction::Unparent, m_targetScene);
			EditorCommandStack::PushRedo(command);

			for (int i = 0; i < myData.size(); i++)
			{
				myData[i]->myChild.UnparentEntity();
				EditorUtils::MarkEntityAndChildrenComponentAsEdited(m_targetScene, myData[i]->myChild, Volt::GetTypeGUID<Volt::TransformComponent>());
				EditorUtils::MarkEntityComponentAsEdited(m_targetScene, myData[i]->myChild, Volt::GetTypeGUID<Volt::RelationshipComponent>());
				EditorUtils::MarkEntityComponentAsEdited(m_targetScene, myData[i]->myParent, Volt::GetTypeGUID<Volt::RelationshipComponent>());
			}
		}
		else if (myAction == ParentingAction::Unparent)
		{
			Ref<ParentingCommand> command = CreateRef<ParentingCommand>(myData, ParentingAction::Parent, m_targetScene);
			EditorCommandStack::PushRedo(command);

			for (int i = 0; i < myData.size(); i++)
			{
				myData[i]->myChild.SetParent(myData[i]->myParent);
				EditorUtils::MarkEntityAndChildrenComponentAsEdited(m_targetScene, myData[i]->myChild, Volt::GetTypeGUID<Volt::TransformComponent>());
				EditorUtils::MarkEntityComponentAsEdited(m_targetScene, myData[i]->myChild, Volt::GetTypeGUID<Volt::RelationshipComponent>());
				EditorUtils::MarkEntityComponentAsEdited(m_targetScene, myData[i]->myParent, Volt::GetTypeGUID<Volt::RelationshipComponent>());
			}
		}
	}

	void Redo() override
	{
		if (myAction == ParentingAction::Parent)
		{
			Ref<ParentingCommand> command = CreateRef<ParentingCommand>(myData, ParentingAction::Unparent, m_targetScene);
			EditorCommandStack::PushUndo(command, true);

			for (int i = 0; i < myData.size(); i++)
			{
				myData[i]->myChild.UnparentEntity();
				EditorUtils::MarkEntityAndChildrenComponentAsEdited(m_targetScene, myData[i]->myChild, Volt::GetTypeGUID<Volt::TransformComponent>());
				EditorUtils::MarkEntityComponentAsEdited(m_targetScene, myData[i]->myChild, Volt::GetTypeGUID<Volt::RelationshipComponent>());
				EditorUtils::MarkEntityComponentAsEdited(m_targetScene, myData[i]->myParent, Volt::GetTypeGUID<Volt::RelationshipComponent>());
			}
		}
		else if (myAction == ParentingAction::Unparent)
		{
			Ref<ParentingCommand> command = CreateRef<ParentingCommand>(myData, ParentingAction::Parent, m_targetScene);
			EditorCommandStack::PushUndo(command, true);

			for (int i = 0; i < myData.size(); i++)
			{
				myData[i]->myChild.SetParent(myData[i]->myParent);
				EditorUtils::MarkEntityAndChildrenComponentAsEdited(m_targetScene, myData[i]->myChild, Volt::GetTypeGUID<Volt::TransformComponent>());
				EditorUtils::MarkEntityComponentAsEdited(m_targetScene, myData[i]->myChild, Volt::GetTypeGUID<Volt::RelationshipComponent>());
				EditorUtils::MarkEntityComponentAsEdited(m_targetScene, myData[i]->myParent, Volt::GetTypeGUID<Volt::RelationshipComponent>());
			}
		}
	}

private:
	Vector<Ref<ParentChildData>> myData;
	ParentingAction myAction;
	Volt::Scene& m_targetScene;
};

enum class AddOrRemoveComponentAction : uint8_t
{
	Add,
	Remove,
};
struct AddOrRemoveComponentCommand : EditorCommand
{
	AddOrRemoveComponentCommand(VoltGUID componentGuid, AddOrRemoveComponentAction action, Volt::Scene& targetScene, Volt::Entity targetEntity) :
		m_componentGuid(componentGuid), m_action(action), m_targetScene(targetScene), m_targetEntity(targetEntity)
	{
		if (m_action == AddOrRemoveComponentAction::Remove)
		{
			VT_ENSURE(m_targetEntity.HasComponent(m_componentGuid));

			//todo_fabian: maybe a bit wasteful to gather whole entity, but just make it work for now
			Volt::EntityDescSerialization::ComponentData data;
			Volt::EntityDescSerialization::GatherComponentData(m_targetEntity, data);

			for (Volt::EntityDescSerialization::ComponentHeader& header : data.headers)
			{
				if (header.componentGUID != m_componentGuid)
				{
					continue;
				}

				uint8_t* const startComponentDataPtr = data.data.data() + header.componentDataOffset;
				m_removedComponentData.resize_uninitialized(header.componentDataSize);
				memcpy_s(m_removedComponentData.data(), m_removedComponentData.size(), startComponentDataPtr, header.componentDataSize);
			}
		}	
	}


	void Execute() override
	{}

	void Undo() override
	{
		if (m_action == AddOrRemoveComponentAction::Add)
		{
			Ref<AddOrRemoveComponentCommand> command = CreateRef<AddOrRemoveComponentCommand>(m_componentGuid, AddOrRemoveComponentAction::Remove, m_targetScene, m_targetEntity);
			EditorCommandStack::PushRedo(command);

			RemoveComponent();
		}
		else if (m_action == AddOrRemoveComponentAction::Remove)
		{
			Ref<AddOrRemoveComponentCommand> command = CreateRef<AddOrRemoveComponentCommand>(m_componentGuid, AddOrRemoveComponentAction::Add, m_targetScene, m_targetEntity);
			EditorCommandStack::PushRedo(command);

			AddComponent();
		}
	}

	void Redo() override
	{
		if (m_action == AddOrRemoveComponentAction::Add)
		{
			Ref<AddOrRemoveComponentCommand> command = CreateRef<AddOrRemoveComponentCommand>(m_componentGuid, AddOrRemoveComponentAction::Remove, m_targetScene, m_targetEntity);
			EditorCommandStack::PushUndo(command, true);

			RemoveComponent();
		}
		else if (m_action == AddOrRemoveComponentAction::Remove)
		{
			Ref<AddOrRemoveComponentCommand> command = CreateRef<AddOrRemoveComponentCommand>(m_componentGuid, AddOrRemoveComponentAction::Add, m_targetScene, m_targetEntity);
			EditorCommandStack::PushUndo(command, true);

			AddComponent();
		}
	}

private:

	void AddComponent()
	{
		if (!Volt::ComponentRegistry::Helpers::HasComponentWithGUID(m_componentGuid, m_targetScene.GetEntityScene().GetRegistry(), m_targetEntity))
		{
			Volt::ComponentRegistry::Helpers::AddComponentWithGUID(m_componentGuid, m_targetScene.GetEntityScene().GetRegistry(), m_targetEntity);

			const Volt::IComponentTypeDesc* componentTypeDesc = reinterpret_cast<const Volt::IComponentTypeDesc*>(Volt::ComponentRegistry::Get().GetTypeDescFromGUID(m_componentGuid));
			if (componentTypeDesc)
			{
				componentTypeDesc->OnInitialize(m_targetEntity);
				EditorUtils::MarkEntityComponentAsEdited(m_targetScene, m_targetEntity, componentTypeDesc->GetGUID());
			}
		}
	}

	void RemoveComponent()
	{
		if (Volt::ComponentRegistry::Helpers::HasComponentWithGUID(m_componentGuid, m_targetScene.GetEntityScene().GetRegistry(), m_targetEntity))
		{
			Volt::ComponentRegistry::Helpers::RemoveComponentWithGUID(m_componentGuid, m_targetScene.GetEntityScene().GetRegistry(), m_targetEntity);
			EditorUtils::MarkEntityComponentAsEdited(m_targetScene, m_targetEntity, m_componentGuid);
		}
	}

	Vector<uint8_t> m_removedComponentData;
	VoltGUID m_componentGuid;
	AddOrRemoveComponentAction m_action;
	Volt::Scene& m_targetScene;
	Volt::Entity m_targetEntity;

	
};
