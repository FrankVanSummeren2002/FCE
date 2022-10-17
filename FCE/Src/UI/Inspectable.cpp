#include "Core/FCEpch.h"
#include "UI\Inspectable.h"
#include "UI/ImGuiHandler.h"



FCE::Inspectable::Inspectable()
{
	ImGuiHandler::AddInspectable(this);
}
void FCE::Inspectable::AddToHandler()
{
	ImGuiHandler::AddInspectable(this);
}
FCE::Inspectable::~Inspectable()
{
	ImGuiHandler::RemoveInspectable(this);
}

void FCE::Inspectable::Display()
{

}

void FCE::Inspectable::SetName(const char* aName)
{
	mName = aName;
}

const char* FCE::Inspectable::GetName()
{
	return mName;
}
