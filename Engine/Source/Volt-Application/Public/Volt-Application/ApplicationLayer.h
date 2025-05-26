#pragma once

namespace Volt
{
	class ApplicationLayer
	{
	public:
		virtual ~ApplicationLayer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
	};
}
