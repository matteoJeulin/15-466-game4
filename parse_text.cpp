#include "parse_text.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>
#include "read_write_chunk.hpp"

Parser::Parser()
{
    FT_Error ft_error;

    if ((ft_error = FT_Init_FreeType(&ft_library)))
    {
        std::cout << "No library" << std::endl;
        abort();
    }
    if ((ft_error = FT_New_Face(ft_library, font_file, 0, &ft_face)))
    {
        std::cout << "No Face " << ft_error << std::endl;
        abort();
    }
    if ((ft_error = FT_Set_Char_Size(ft_face, font_size * 64, font_size * 64, 0, 0)))
    {
        std::cout << "No Char size" << std::endl;
        abort();
    }

    hb_font = hb_ft_font_create(ft_face, NULL);
    story = StateMachine();
}

Parser::~Parser()
{
    hb_font_destroy(hb_font);

    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_library);
}

void Parser::parse_atlas(const std::string &filename)
{
    std::ifstream file(filename);

    char line[max_line_length];
    while (file.getline(line, max_line_length))
    {
        get_meshes_from_line(line);
    }
    file.close();

    std::string atlas = std::filesystem::path(filename).stem();
    std::ofstream out("./parsing/" + atlas + ".atlas", std::ios::binary);
    write_chunk("mesh", character_atlas_vector, &out);
    out.close();
}

void Parser::get_meshes_from_line(char *line)
{
    hb_buffer_t *hb_buffer;
    hb_buffer = hb_buffer_create();
    hb_buffer_add_utf8(hb_buffer, line, -1, 0, -1);
    hb_buffer_guess_segment_properties(hb_buffer);

    hb_shape(hb_font, hb_buffer, NULL, 0);

    unsigned int len = hb_buffer_get_length(hb_buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, NULL);
    // hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, NULL);

    for (unsigned int i = 0; i < len; i++)
    {
        hb_codepoint_t gid = info[i].codepoint;

        if (gid == 0)
        {
            throw std::invalid_argument("File contained characters not defined in the given font");
        }

        if (character_atlas.find(gid) == character_atlas.end())
        {
            FT_Load_Glyph(ft_face, gid, FT_LOAD_DEFAULT);

            FT_GlyphSlot slot = ft_face->glyph;
            FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

            FT_Bitmap bitmap = slot->bitmap;

            character_atlas.emplace(std::pair(gid, bitmap));
            character_atlas_vector.emplace_back(std::pair(gid, bitmap));
        }
    }

    hb_buffer_destroy(hb_buffer);
}

void Parser::parse_story(const std::string &filename)
{
    std::ifstream file(filename);

    // Get the number of states from the file
    uint32_t nb_states;
    file >> nb_states;

    uint32_t state_id;
    while (file >> state_id)
    {
        State current_state;
        current_state.id = state_id;

        char c;
        file >> c;
        // Checking proper formatting
        if (c != '{')
        {
            std::string err = "Improper formatting, was expecting '{', got '";
            err += c;
            err.append("'.");
            throw std::invalid_argument(err);
        }

        // Get the state text
        file.getline(current_state.text, max_line_length, '|');
        std::cout << current_state.text << std::endl;

        // Get the transitions form current_state
        uint32_t transition_index = 0;
        c = file.peek();
        while (c != '}')
        {
            assert(transition_index < max_transition && "Too many transitions from current state.");
            Transition t;
            
            file >> t.dst_id;

            // Get the transition text
            file.getline(t.text, max_line_length, '|');
            current_state.transitions[transition_index] = t;
            std::cout << t.text << std::endl;

            transition_index++;
            c = file.peek();
        }
        file >> c;

        story.states.emplace_back(current_state);
    }

    assert(story.states.size() == nb_states && "Invalid number of states in the state machine, does not match number specified at the top of the file");

    std::string atlas = std::filesystem::path(filename).stem();
    std::ofstream out("./parsing/" + atlas + ".story", std::ios::binary);
    write_chunk("stry", story.states, &out);
    out.close();

    file.close();
}

StateMachine::StateMachine()
{
    states = std::vector<State>();
}

StateMachine::StateMachine(std::vector<State> _states)
{
    states = std::vector(_states);
}

int main()
{
    Parser parser;
    parser.parse_atlas("test.txt");
    parser.parse_story("test.txt");
}