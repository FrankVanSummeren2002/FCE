#include "Core/FCEpch.h"
#include "UI/DebugInspectable.h"
#include "UI/ImGuiHandler.h"



FCE::DebugInspectable::DebugInspectable()
{
	ImGuiHandler::AddInspectable(this);
}
void FCE::DebugInspectable::AddToHandler()
{
	ImGuiHandler::AddInspectable(this);
}
FCE::DebugInspectable::~DebugInspectable()
{
	ImGuiHandler::RemoveInspectable(this);
}

void FCE::DebugInspectable::Display()
{

}

void FCE::DebugInspectable::SetName(const char* aName)
{
	mName = aName;
}

const char* FCE::DebugInspectable::GetName()
{
	return mName;
}
