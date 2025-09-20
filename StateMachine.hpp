#pragma once

#include <vector>
#include <string>
#include <stdint.h>

constexpr uint32_t max_line_length = 128;
constexpr uint32_t max_transition = 2;

// Transition struct for the state machine
struct Transition
{
    uint32_t dst_id = -1;                  // Id of the state to go to
    char text[max_line_length] = {0}; // Text to display on transition
};

// State struct for the state machine
struct State
{
    uint32_t id = -1;                             // Index of the current state in the state machine
    char text[max_line_length] = {0};             // Text to display on arriving to this state
    Transition transitions[max_transition]; // Transitions from this state
};

// State machine
struct StateMachine
{
    std::vector<State> states;

    StateMachine();
    StateMachine(std::vector<State> states);
};