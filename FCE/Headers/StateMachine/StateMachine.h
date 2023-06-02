#pragma once
#include <unordered_map>
#include <string>
class State;

class StateMachine
{
	State* mCurrentState = nullptr;

	std::unordered_map<std::string, State*> mStates;

public:
	//add an state to the state machine
	bool AddState(State* aState);
	bool Update(float aDelta);
	//takes the string name of the state you want to switch to
	//returns false if it could not find the state
	bool ChangeState(std::string aNewState);
	//delete all states
	bool DeleteStates();
	//get the current state
	State* GetGurrentState();
	//get the state with a specific name
	//returns a nullptr if the statemachine does not have it
	State* GetState(std::string);
	//get the name of the current 
	std::string GetCurrentName();
	~StateMachine();
};