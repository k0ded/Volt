#pragma once

#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Core/RHIInterface.h"

struct GLFWwindow;

typedef void* ImTextureID;
struct ImFont;
struct ImGuiContext;

namespace Volt::RHI
{
	class Swapchain;
	class Image;

	struct ImGuiCreateInfo
	{
		GLFWwindow* window = nullptr;
		RawPtr<Swapchain> swapchain;
		bool enableViewports = true;
	};

	class VTRHI_API ImGuiImplementation : public RHIInterface
	{
	public:
		struct FontInfo
		{
			std::filesystem::path filepath;
			float pixelSize;
		};

		virtual ~ImGuiImplementation();

		VT_DELETE_COPY_MOVE(ImGuiImplementation);

		void Begin();
		void End();

		void SetDefaultFont(ImFont* font);
		ImGuiContext* GetContext() const;

		virtual ImTextureID GetTextureID(RefPtr<Image> image, int32_t mipIndex = -1) const = 0;
		virtual ImFont* AddFont(const std::filesystem::path& fontPath, float pixelSize) = 0;
		virtual Vector<ImFont*> AddFonts(const Vector<FontInfo>& fontInfos) = 0;

		static RefPtr<ImGuiImplementation> Create(const ImGuiCreateInfo& createInfo);
		static ImGuiImplementation& Get();


	protected:
		ImGuiImplementation(ImGuiCreateInfo createInfo);

		void Initialize();

		virtual void BeginAPI() = 0;
		virtual void EndAPI() = 0;

		virtual void InitializeAPI(ImGuiContext* context) {}
		virtual void ShutdownAPI() {}

	private:
		void CreateContext();
		inline static ImGuiImplementation* s_instance = nullptr;
		ImFont* m_defaultFont = nullptr;
		ImGuiCreateInfo m_createInfo;

		ImGuiContext* m_context = nullptr;
	};
}
