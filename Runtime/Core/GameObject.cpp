/*
Copyright(c) 2016-2017 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES ============================
#include "GameObject.h"
#include "Scene.h"
#include "GUIDGenerator.h"
#include "../IO/StreamIO.h"
#include "../Logging/Log.h"
#include "../Components/AudioSource.h"
#include "../Components/AudioListener.h"
#include "../Components/Camera.h"
#include "../Components/Collider.h"
#include "../Components/Transform.h"
#include "../Components/Hinge.h"
#include "../Components/Light.h"
#include "../Components/LineRenderer.h"
#include "../Components/MeshFilter.h"
#include "../Components/MeshRenderer.h"
#include "../Components/RigidBody.h"
#include "../Components/Skybox.h"
#include "../Components/Script.h"
//======================================

//= NAMESPACES =====
using namespace std;
//==================

namespace Directus
{
	GameObject::GameObject(Context* context)
	{
		m_context = context;
		m_ID = GENERATE_GUID;
		m_name = "GameObject";
		m_isActive = true;
		m_isPrefab = false;
		m_hierarchyVisibility = true;
		m_transform = nullptr;
		m_meshFilter = nullptr;
		m_meshRenderer = nullptr;
	}

	GameObject::~GameObject()
	{
		// delete components
		for (int i = 0; i < m_components.size(); i++)
		{
			delete m_components[i];
		}
		m_components.clear();

		m_ID = NOT_ASSIGNED_HASH;
		m_name.clear();
		m_isActive = true;
		m_hierarchyVisibility = true;
	}

	void GameObject::Initialize(Transform* transform)
	{
		m_transform = transform;
	}

	void GameObject::Start()
	{
		// call component Start()
		for (auto const& component : m_components)
		{
			component->Start();
		}
	}

	void GameObject::OnDisable()
	{
		// call component OnDisable()
		for (auto const& component : m_components)
		{
			component->OnDisable();
		}
	}

	void GameObject::Update()
	{
		if (!m_isActive)
			return;

		// call component Update()
		for (const auto& component : m_components)
		{
			component->Update();
		}
	}

	bool GameObject::SaveAsPrefab(const string& filePath)
	{
		// Create a prefab file
		unique_ptr<StreamIO> file = make_unique<StreamIO>(filePath + PREFAB_EXTENSION, Mode_Write);
		if (!file->IsCreated())
			return false;

		// Serialize
		m_isPrefab = true;
		Serialize(file.get());

		return true;
	}

	bool GameObject::LoadFromPrefab(const string& filePath)
	{
		// Make sure that this is a prefab file
		if (!FileSystem::IsEnginePrefabFile(filePath))
			return false;

		// Try to open it
		unique_ptr<StreamIO> file = make_unique<StreamIO>(filePath, Mode_Read);
		if (!file->IsCreated())
			return false;

		Deserialize(file.get(), nullptr);

		return true;
	}

	void GameObject::Serialize(StreamIO* stream)
	{
		//= BASIC DATA ==========================
		stream->Write(m_isPrefab);
		stream->Write(m_isActive);
		stream->Write(m_hierarchyVisibility);
		stream->Write(m_ID);
		stream->Write(m_name);
		//=======================================

		//= COMPONENTS ================================
		stream->Write((int)m_components.size());
		for (const auto& component : m_components)
		{
			stream->Write(component->g_typeStr);
			stream->Write(component->g_ID);
		}

		for (const auto& component : m_components)
		{
			component->Serialize(stream);
		}
		//=============================================

		//= CHILDREN ==================================
		vector<Transform*> children = GetTransform()->GetChildren();

		// 1st - children count
		stream->Write((int)children.size());

		// 2nd - children IDs
		for (const auto& child : children)
		{
			stream->Write(child->g_ID);
		}

		// 3rd - children
		for (const auto& child : children)
		{
			if (!child->g_gameObject.expired())
			{
				child->g_gameObject._Get()->Serialize(stream);
			}
			else
			{
				LOG_ERROR("Aborting GameObject serialization, child GameObject is nullptr.");
				break;
			}
		}
		//=============================================
	}

	void GameObject::Deserialize(StreamIO* stream, Transform* parent)
	{
		//= BASIC DATA =====================
		stream->Read(m_isPrefab);
		stream->Read(m_isActive);
		stream->Read(m_hierarchyVisibility);
		stream->Read(m_ID);
		stream->Read(m_name);
		//==================================

		//= COMPONENTS ================================
		int componentCount = stream->ReadInt();
		for (int i = 0; i < componentCount; i++)
		{
			string type = NOT_ASSIGNED;
			unsigned int id = 0;

			stream->Read(type); // load component's type
			stream->Read(id); // load component's id

			Component* component = AddComponentBasedOnType(type);
			component->g_ID = id;
		}
		// Sometimes there are component dependencies, e.g. a collider that needs
		// to set it's shape to a rigibody. So, it's important to first create all 
		// the components (like above) and then deserialize them (like here).
		for (const auto& component : m_components)
		{
			component->Deserialize(stream);
		}
		//=============================================

		// Set the transform's parent
		if (m_transform)
		{
			m_transform->SetParent(parent);
		}

		//= CHILDREN ===================================
		// 1st - children count
		int childrenCount = stream->ReadInt();

		// 2nd - children IDs
		auto scene = m_context->GetSubsystem<Scene>();
		vector<weakGameObj> children;
		for (int i = 0; i < childrenCount; i++)
		{
			weakGameObj child = scene->CreateGameObject();
			child._Get()->SetID(stream->ReadUInt());
			children.push_back(child);
		}

		// 3rd - children
		for (const auto& child : children)
		{
			child._Get()->Deserialize(stream, GetTransform());
		}
		//=============================================

		if (m_transform)
		{
			m_transform->ResolveChildrenRecursively();
		}
	}

	void GameObject::RemoveComponentByID(unsigned int id)
	{
		for (auto it = m_components.begin(); it != m_components.end(); ) 
		{
			auto component = *it;
			if (id == component->g_ID)
			{
				delete component;
				it = m_components.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	//= HELPER FUNCTIONS ===========================================
	Component* GameObject::AddComponentBasedOnType(const string& typeStr)
	{
		// Note: this is the only hardcoded part regarding
		// components. It's one function but it would be
		// nice if that get's automated too.

		Component* component = nullptr;

		if (typeStr == "Transform")
			component = AddComponent<Transform>();

		if (typeStr == "MeshFilter")
			component = AddComponent<MeshFilter>();

		if (typeStr == "MeshRenderer")
			component = AddComponent<MeshRenderer>();

		if (typeStr == "Light")
			component = AddComponent<Light>();

		if (typeStr == "Camera")
			component = AddComponent<Camera>();

		if (typeStr == "Skybox")
			component = AddComponent<Skybox>();

		if (typeStr == "RigidBody")
			component = AddComponent<RigidBody>();

		if (typeStr == "Collider")
			component = AddComponent<Collider>();

		if (typeStr == "Hinge")
			component = AddComponent<Hinge>();

		if (typeStr == "Script")
			component = AddComponent<Script>();

		if (typeStr == "LineRenderer")
			component = AddComponent<LineRenderer>();

		if (typeStr == "AudioSource")
			component = AddComponent<AudioSource>();

		if (typeStr == "AudioListener")
			component = AddComponent<AudioListener>();

		return component;
	}
}
