#pragma once

namespace FCE
{
	struct InputComponent
	{
		std::function<void(entt::entity,float DT)> mCallback;
		InputComponent(std::function<void(entt::entity,float DT)> callback)
		{
			mCallback = callback;
		};
	};
}