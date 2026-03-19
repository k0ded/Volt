#pragma once

#include <RenderDoc/renderdoc_app.h>

#include <LogModule/LogCategory.h>
#include <EventSystem/EventListener.h>
#include <Volt-Core/Plugin/Plugin.h>

#include <CoreUtilities/Pointers/Unique.h>

VT_DECLARE_LOG_CATEGORY(LogRenderDoc, LogVerbosity::Trace);

class RenderDocFrameCapture;

class RenderDocEventListener : public Volt::EventListener
{
public:
	~RenderDocEventListener() override;

	void RegisterListeners(Ref<RenderDocFrameCapture> frameCapture);

private:
	Ref<RenderDocFrameCapture> m_frameCapture;
	bool m_isCapturing = false;
};

class PLUGIN_API RenderDocPlugin : public Volt::Plugin
{
public:
	RenderDocPlugin();
	~RenderDocPlugin() override;

	VT_DECLARE_PLUGIN_GUID("5AFA1093-1162-4C90-A2E8-946001C427E8"_guid);

	inline uint32_t GetVersion() const override { return 1; }
	inline std::string_view GetName() const override { return "RenderDocPlugin"; }
	inline std::string_view GetDescription() const override { return "RenderDoc"; }
	inline std::string_view GetCategory() const override { return "None"; }

	void Initialize() override;
	void Shutdown() override;

private:
	Ref<RenderDocFrameCapture> m_frameCapture;
	Unique<RenderDocEventListener> m_eventListener;

	void* m_renderDocModule = nullptr;
	RENDERDOC_API_1_6_0* m_renderDocAPI = nullptr;
};

VT_REGISTER_PLUGIN(RenderDocPlugin);
