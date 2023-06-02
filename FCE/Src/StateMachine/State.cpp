#include "StateMachine/State.h"
#include "StateMachine/StateMachine.h"

void State::SetStateMachine(StateMachine* aStateMachine)
{
	mStateMachine = aStateMachine;
}

bool State::AddStateAndTransition(std::string aName, std::function<bool(State* aState)> aState)
{
	// cant create a transition to yourself
	if (aName == mName)
		return false;
	//if there already is a transtion towards that state ignore it. 
	if (mSwitchStates.find(aName) == mSwitchStates.end())
	{
		mSwitchStates.insert(std::pair(aName, aState));
		return true;
	}
	return false;
}
bool State::CheckTransitions()
{
	//loop over all the transitions
	for (auto i = mSwitchStates.begin(); i != mSwitchStates.end(); ++i)
	{
		//call the function with yourself as argument
		if (i->second(this))
		{
			//if the transition returns true swith to the state from that transition
			mStateMachine->ChangeState(i->first);
			return true;
		}
	}
	return false;
}
void State::SetName(std::string aName)
{
	mName = aName;
}
std::string State::GetName()
{
	return mName;
}
bool State::Tick(float DT)
{
	//first do the transitions then the update function if it has not switched;
	if (!CheckTransitions())
		return Update(DT);
	return false;
}
void State::Draw() {}