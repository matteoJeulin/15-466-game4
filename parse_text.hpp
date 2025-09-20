#pragma once

// FreeType
#include <ft2build.h>
#include FT_FREETYPE_H

// HarfBuzz
#include <hb.h>
#include <hb-ft.h>

#include <unordered_map>
#include <vector>
#include <string>

// #include "StateMachine.hpp"

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

struct Parser
{
    struct CharacterMesh
    {
    };

    // Map of all previously seen characters and their corresponding mesh
    std::unordered_map<hb_codepoint_t, FT_Bitmap> character_atlas;
    std::vector<std::pair<hb_codepoint_t, FT_Bitmap>> character_atlas_vector;

    // State machine for the story
    StateMachine story;

    const char *font_file = "FreeSans.otf";

    const int font_size = 36;
    const int margin = font_size / 2;

    FT_Library ft_library;
    FT_Face ft_face;
    hb_font_t *hb_font;

    // Parse the character atlas from a file
    void parse_atlas(const std::string &filename);

    // Parse a state machine from a file.
    // The expected format is the following:
    // n
    // ...
    // i {text|j transition_text|k transition_text|...|}
    // ...
    // With i the id of the current state (states should be written in order), text the text to show
    // when arriving in state i, j the state to transition to and transition text the text to show
    // when using this transition.
    void parse_story(const std::string &filename);

    void get_meshes_from_line(char *line);

    Parser();
    ~Parser();
};
