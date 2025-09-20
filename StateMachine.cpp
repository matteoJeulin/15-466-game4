#include "StateMachine.hpp"

StateMachine::StateMachine() {
    states = std::vector<State>();
}

StateMachine::StateMachine(std::vector<State> _states) {
    this->states = std::vector(_states);
}