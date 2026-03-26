#include "cupch.h"
#include "DynamicLibraryHelpers.h"

DynamicLibraryHelper::DynamicLibraryHelper(WStringView libraryFilepath)
	: m_moduleHandle(nullptr)
{
	if (!libraryFilepath.empty())
	{
		m_moduleHandle = VT_LOAD_LIBRARY(libraryFilepath.data());
	}
}

DynamicLibraryHelper::~DynamicLibraryHelper()
{
	Free();
}

void DynamicLibraryHelper::Free()
{
	if (m_moduleHandle != nullptr)
	{
		VT_FREE_LIBRARY(m_moduleHandle);
		m_moduleHandle = nullptr;
	}
}

void DynamicLibraryHelper::Release()
{
	m_moduleHandle = nullptr;
}

void DynamicLibraryHelper::Load(WStringView libraryFilepath)
{
	if (m_moduleHandle != nullptr)
	{
		VT_FREE_LIBRARY(m_moduleHandle);
		m_moduleHandle = nullptr;
	}

	if (!libraryFilepath.empty())
	{
		m_moduleHandle = VT_LOAD_LIBRARY(libraryFilepath.data());
	}
}
