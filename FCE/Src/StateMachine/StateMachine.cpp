#include "StateMachine/StateMachine.h"
#include "StateMachine/State.h"

bool StateMachine::AddState(State* aState)
{
	//you cant add a state if it is already there or it does not have a preset name
	if (mStates.find(aState->mName) != mStates.end() || aState->GetName() == "NULL")
	{
		return false;
	}
	mStates.insert(std::pair(aState->mName, aState));
	aState->SetStateMachine(this);
	return true;
}

bool StateMachine::Update(float aDt)
{
	return mCurrentState->Tick(aDt);
}

bool StateMachine::ChangeState(std::string aNewState)
{
	//see if the state machine has that state
	auto state = mStates.find(aNewState);
	if (state != mStates.end())
	{
		if (mCurrentState != state->second)
		{
			//if the current state is not a nullptr call the OnEnd function
			if (mCurrentState != nullptr)
				mCurrentState->OnEnd();
			//set new state to the current state and call onEnter
			mCurrentState = state->second;
			mCurrentState->OnEnter();
			return true;
		}
	}
	return false;
}

bool StateMachine::DeleteStates()
{
	//set the current state to a nullptr
	mCurrentState = nullptr;
	//delete every state
	for (std::pair<std::string, State*> element : mStates)
	{
		delete element.second;
	}
	mStates.clear();
	return true;
}

State* StateMachine::GetGurrentState()
{
	return mCurrentState;
}

State* StateMachine::GetState(std::string name)
{
	auto state = mStates.find(name);
	if (state == mStates.end())
		return nullptr;
	return state->second;
}


std::string StateMachine::GetCurrentName()
{
	if (mCurrentState == nullptr)
		return std::string("NULL");
	return mCurrentState->mName;
}

StateMachine::~StateMachine()
{
	DeleteStates();
}

