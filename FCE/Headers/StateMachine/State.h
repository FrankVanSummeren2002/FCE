#pragma once
#include <unordered_map>
#include <functional>


class StateMachine;
class State
{
	//all of the state switches
	std::unordered_map<std::string, std::function<bool(State* aState)>>mSwitchStates;
public:
	StateMachine* mStateMachine = nullptr;
	std::string mName = "NULL";

	void SetStateMachine(StateMachine* aStateMachine);
	//add a transition function and the name of the state you want to switch to
	bool AddStateAndTransition(std::string, std::function<bool(State* aState)> aState);
	bool Tick(float aDt);

	virtual void Draw();

	bool CheckTransitions();
	virtual void OnEnter() = 0;
	virtual bool Update(float aDt) = 0;
	virtual void OnEnd() = 0;
	virtual ~State() {}
	void SetName(std::string aName);
	std::string GetName();
};
