#pragma once

#include <Volt/Core/Base.h>
#include <Volt-Scene/Components/CoreComponents.h>
#include <Volt-Scene/Entity.h>
#include <Volt-Scene/SceneSerializer.h>

#include <CoreUtilities/FileIO/YAMLMemoryStreamWriter.h>
#include <CoreUtilities/FileIO/YAMLMemoryStreamReader.h>


#include <stack>
#include "EditorCommand.h"


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
	ValueCommand(T* aValueAdress, T aPreviousValue) : myValueAdress(aValueAdress), myPreviousValue(aPreviousValue) {}
	void Execute() override {}

	void Undo() override
	{
		Ref<ValueCommand<T>> command = CreateRef<ValueCommand<T>>(myValueAdress, *myValueAdress);
		EditorCommandStack::GetInstance().PushRedo(command);
		*myValueAdress = myPreviousValue;
	}

	void Redo() override
	{
		Ref<ValueCommand<T>> command = CreateRef<ValueCommand<T>>(myValueAdress, *myValueAdress);
		EditorCommandStack::GetInstance().PushUndo(command, true);
		*myValueAdress = myPreviousValue;
	}

private:
	T* myValueAdress;
	const T myPreviousValue;
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

		Weak<Volt::Scene> scene;
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
	Weak<Volt::Scene> myScene;
};

struct MultiGizmoCommand : EditorCommand
{
	MultiGizmoCommand(Weak<Volt::Scene> scene, const Vector<std::pair<Volt::EntityID, Volt::TransformComponent>>& entities)
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
		}

		Ref<MultiGizmoCommand> command = CreateRef<MultiGizmoCommand>(myScene, currentTransforms);
		EditorCommandStack::GetInstance().PushUndo(command);
	}

private:
	Vector<std::pair<Volt::EntityID, Volt::TransformComponent>> myPreviousTransforms;
	Weak<Volt::Scene> myScene;
};

enum class ObjectStateAction
{
	Create,
	Delete
};

struct ObjectStateCommand : EditorCommand
{
	ObjectStateCommand(Vector<Volt::Entity> entityList, ObjectStateAction action) :
		m_Action(action)
	{
		VT_ENSURE(entityList.size() > 0);
		m_TargetScene = entityList[0].GetScene();

		m_EntityIDs.reserve(entityList.size());
		for (Volt::Entity& entity : entityList)
		{
			m_EntityIDs.push_back(entity.GetID());

			//if the action is delete, we need to save the data so that we can recreate the actor later
			if (action == ObjectStateAction::Delete)
			{
				SerializeEntityToDataList(entity);
			}
		}
	}

	ObjectStateCommand(Volt::Entity entity, ObjectStateAction action) :
		m_Action(action)
	{
		m_EntityIDs.push_back(entity.GetID());

		m_TargetScene = entity.GetScene();

		//if the action is delete, we need to save the data so that we can recreate the actor later
		if (action == ObjectStateAction::Delete)
		{
			SerializeEntityToDataList(entity);
		}
	}

	ObjectStateCommand(Vector<Volt::EntityID> entityIDs, Weak<Volt::Scene> targetScene, ObjectStateAction action) :
		m_EntityIDs(entityIDs), m_TargetScene(targetScene), m_Action(action)
	{
		//if the action is delete, we need to save the data so that we can recreate the actor later
		if (m_Action == ObjectStateAction::Delete)
		{
			for (Volt::EntityID& id : m_EntityIDs)
			{
				Volt::Entity entity = m_TargetScene->GetEntityFromID(id);
				SerializeEntityToDataList(entity);
			}
		}
	}

	void Execute() override
	{}

	void Undo() override
	{
		if (m_Action == ObjectStateAction::Create)
		{
			Ref<ObjectStateCommand> command = CreateRef<ObjectStateCommand>(m_EntityIDs, m_TargetScene, ObjectStateAction::Delete);
			EditorCommandStack::PushRedo(command);

			for (Volt::EntityID& id : m_EntityIDs)
			{
				Volt::Entity entity = m_TargetScene->GetEntityFromID(id);
				m_TargetScene->DestroyEntity(entity);
			}
		}
		else if (m_Action == ObjectStateAction::Delete)
		{
			CreateEntitiesFromDataList();

			Ref<ObjectStateCommand> command = CreateRef<ObjectStateCommand>(m_EntityIDs, m_TargetScene, ObjectStateAction::Create);
			EditorCommandStack::PushRedo(command);
		}
	}

	void Redo() override
	{
		//treat the Redo as an Undo

		if (m_Action == ObjectStateAction::Create)
		{
			Ref<ObjectStateCommand> command = CreateRef<ObjectStateCommand>(m_EntityIDs, m_TargetScene, ObjectStateAction::Delete);
			EditorCommandStack::PushUndo(command, true);

			for (Volt::EntityID& id : m_EntityIDs)
			{
				Volt::Entity entity = m_TargetScene->GetEntityFromID(id);
				m_TargetScene->DestroyEntity(entity);
			}
		}
		else if (m_Action == ObjectStateAction::Delete)
		{
			CreateEntitiesFromDataList();

			Ref<ObjectStateCommand> command = CreateRef<ObjectStateCommand>(m_EntityIDs, m_TargetScene, ObjectStateAction::Create);
			EditorCommandStack::PushUndo(command, true);

		}
	}

private:
	void SerializeEntityToDataList(Volt::Entity entity)
	{
		YAMLMemoryStreamWriter writer{};

		Volt::AssetMetadata fakeMetadata;
		fakeMetadata.filePath = "Metadata Created By ObjectStateCommand.";
		Volt::SceneSerializer::Get().SerializeEntity(entity.GetHandle(), fakeMetadata, entity.GetScene(), writer);

		m_EntitiesDataList.push_back(writer.WriteAndGetBuffer());

		writer.WriteAndGetBuffer();
	}

	void CreateEntitiesFromDataList()
	{
		for (Buffer& buffer : m_EntitiesDataList)
		{
			YAMLMemoryStreamReader reader;
			reader.ConsumeBuffer(buffer);
			Volt::AssetMetadata fakeMetadata;
			fakeMetadata.filePath = "Metadata Created By ObjectStateCommand.";
			Volt::SceneSerializer::Get().DeserializeEntity(m_TargetScene, fakeMetadata, reader);
		}

		//have to invalidate the transform after spawning the entity
		//todo: make it so we dont need to manually invalidate
		for (Volt::EntityID& id : m_EntityIDs)
		{
			m_TargetScene->InvalidateEntityTransform(id);
		}
	}

	Weak<Volt::Scene> m_TargetScene;
	Vector<Volt::EntityID> m_EntityIDs;
	Vector<Buffer> m_EntitiesDataList;
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
	ParentingCommand(Vector<Ref<ParentChildData>> aData, ParentingAction anAction) :
		myData(aData), myAction(anAction)
	{}

	void Execute() override
	{}

	void Undo() override
	{
		if (myAction == ParentingAction::Parent)
		{
			Ref<ParentingCommand> command = CreateRef<ParentingCommand>(myData, ParentingAction::Unparent);
			EditorCommandStack::PushRedo(command);

			for (int i = 0; i < myData.size(); i++)
			{
				myData[i]->myChild.GetScene()->UnparentEntity(myData[i]->myChild);
			}
		}
		else if (myAction == ParentingAction::Unparent)
		{
			Ref<ParentingCommand> command = CreateRef<ParentingCommand>(myData, ParentingAction::Parent);
			EditorCommandStack::PushRedo(command);

			for (int i = 0; i < myData.size(); i++)
			{
				myData[i]->myChild.GetScene()->ParentEntity(myData[i]->myParent, myData[i]->myChild);
			}
		}
	}

	void Redo() override
	{
		if (myAction == ParentingAction::Parent)
		{
			Ref<ParentingCommand> command = CreateRef<ParentingCommand>(myData, ParentingAction::Unparent);
			EditorCommandStack::PushUndo(command, true);

			for (int i = 0; i < myData.size(); i++)
			{
				myData[i]->myChild.GetScene()->UnparentEntity(myData[i]->myChild);
			}
		}
		else if (myAction == ParentingAction::Unparent)
		{
			Ref<ParentingCommand> command = CreateRef<ParentingCommand>(myData, ParentingAction::Parent);
			EditorCommandStack::PushUndo(command, true);

			for (int i = 0; i < myData.size(); i++)
			{
				myData[i]->myChild.GetScene()->ParentEntity(myData[i]->myParent, myData[i]->myChild);
			}
		}
	}

private:
	Vector<Ref<ParentChildData>> myData;
	ParentingAction myAction;
};
