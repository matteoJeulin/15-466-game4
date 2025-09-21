#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <fstream>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load<MeshBuffer> hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const *
								{
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

Load<Scene> hexapod_scene(LoadTagDefault, []() -> Scene const *
						  { return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name)
											 {
												 Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

												 scene.drawables.emplace_back(transform);
												 Scene::Drawable &drawable = scene.drawables.back();

												 drawable.pipeline = lit_color_texture_program_pipeline;

												 drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
												 drawable.pipeline.type = mesh.type;
												 drawable.pipeline.start = mesh.start;
												 drawable.pipeline.count = mesh.count; }); });

Load<Sound::Sample> dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const *
									   { return new Sound::Sample(data_path("dusty-floor.opus")); });

Load<Sound::Sample> honk_sample(LoadTagDefault, []() -> Sound::Sample const *
								{ return new Sound::Sample(data_path("honk.wav")); });

Load<StateMachine> story_states(LoadTagDefault, []() -> StateMachine const *
								{
	std::vector<StateMachine::State> states;

	std::ifstream file("./parsing/test.story", std::ios::binary);
	read_chunk(file, "stry", &states);

	return new StateMachine(states); });

PlayMode::PlayMode() : scene(*hexapod_scene)
{
	std::cout << "In playmode" << std::endl;
	// get pointers to leg for convenience:
	for (auto &transform : scene.transforms)
	{
		if (transform.name == "Hip.FL")
			hip = &transform;
		else if (transform.name == "UpperLeg.FL")
			upper_leg = &transform;
		else if (transform.name == "LowerLeg.FL")
			lower_leg = &transform;
	}
	if (hip == nullptr)
		throw std::runtime_error("Hip not found.");
	if (upper_leg == nullptr)
		throw std::runtime_error("Upper leg not found.");
	if (lower_leg == nullptr)
		throw std::runtime_error("Lower leg not found.");

	hip_base_rotation = hip->rotation;
	upper_leg_base_rotation = upper_leg->rotation;
	lower_leg_base_rotation = lower_leg->rotation;

	story = StateMachine(story_states.value);

	// get pointer to camera for convenience:
	if (scene.cameras.size() != 1)
		throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	// start music loop playing:
	//  (note: position will be over-ridden in update())
	leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);
}

PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

	if (evt.type == SDL_EVENT_KEY_DOWN)
	{
		if (evt.key.key == SDLK_LEFT)
		{
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.key == SDLK_RIGHT)
		{
			right.downs += 1;
			right.pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_KEY_UP)
	{
		if (evt.key.key == SDLK_LEFT)
		{
			left.pressed = false;
			return false;
		}
		else if (evt.key.key == SDLK_RIGHT)
		{
			right.pressed = false;
			return false;
		}
	}

	return false;
}

void PlayMode::update(float elapsed)
{
	{
		if (left.pressed)
		{
			left.pressed = false;
			story.switch_state(story.current_state.transitions[0]);
		}
		else if (right.pressed)
		{
			right.pressed = false;
			story.switch_state(story.current_state.transitions[1]);
		}
	}

	// reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	// update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	// set up light type and position for lit_color_texture_program:
	//  TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, -1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); // this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ // use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		story.draw_current_state(drawable_size, glm::vec2(10.0f, 10.0f), glm::vec3(1.0f, 1.0f, 1.0f));
	}
	GL_ERRORS();
}

glm::vec3 PlayMode::get_leg_tip_position()
{
	// the vertex position here was read from the model in blender:
	return lower_leg->make_world_from_local() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}
