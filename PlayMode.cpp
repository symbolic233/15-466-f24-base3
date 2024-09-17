#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint piano_textures = 0;
Load< MeshBuffer > piano_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("piano.pnct"));
	piano_textures = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > piano_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("piano.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = piano_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = piano_textures;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
		
		drawable.name = mesh_name;
	});
});

Load< Sound::Sample > correct_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("notes_correct.wav"));
});
Load< Sound::Sample > incorrect_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("notes_incorrect.wav"));
});

PlayMode::PlayMode() : scene(*piano_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "C4") c4 = &transform;
	}
	if (c4 == nullptr) throw std::runtime_error("C4 not found.");
	
	// apply tint function
	for (auto &drawable : scene.drawables) {
		drawable.pipeline.set_uniforms = [&]() {
			for (uint32_t i = 0; i < keycount; i++) {
				if (drawable.name == keys[i]) {
					if (selection & (1 << i)) {
						// different highlighting for white/black keys
						if (drawable.name[1] == 's') glUniform3f(drawable.pipeline.TINT_vec3, cycle * 0.5f, 3.0f - 2.0f * cycle, cycle * 0.5f);
						else glUniform3f(drawable.pipeline.TINT_vec3, cycle * 0.5f, 1.0f, cycle * 0.5f);
					}
					else {
						glUniform3f(drawable.pipeline.TINT_vec3, 1.0f, 1.0f, 1.0f);
					}
				}
			}
		};
	}

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	correct_loop = Sound::loop(*correct_sample, 1.0f, 0.0f);
	incorrect_loop = Sound::loop(*incorrect_sample, 1.0f, 0.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LSHIFT || evt.key.keysym.sym == SDLK_RSHIFT) {
			sharp = true;
			return true;
		}
		std::string choice = "";
		if (evt.key.keysym.sym == SDLK_a) choice += "A";
		else if (evt.key.keysym.sym == SDLK_b) choice += "B";
		else if (evt.key.keysym.sym == SDLK_c) choice += "C";
		else if (evt.key.keysym.sym == SDLK_d) choice += "D";
		else if (evt.key.keysym.sym == SDLK_e) choice += "E";
		else if (evt.key.keysym.sym == SDLK_f) choice += "F";
		else if (evt.key.keysym.sym == SDLK_g) choice += "G";
		else return false;

		if (sharp) {
			if (choice != "E" && choice != "B") choice += "s"; // there is no E# or B#
			else return false;
		}
		choice += "4";
		std::cout << choice << std::endl;

		for (uint32_t i = 0; i < keycount; i++) {
			if (choice == keys[i]) {
				selection ^= 1 << i;
				std::cout << selection << std::endl;
			}
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LSHIFT || evt.key.keysym.sym == SDLK_RSHIFT) {
			sharp = false;
			return true;
		}
	}
	/* else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	} */

	return false;
}

void PlayMode::update(float elapsed) {

	cycle += elapsed / 1.5f;
	cycle -= std::floor(cycle);

	/*
	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 7.5f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}*/

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	}
	GL_ERRORS();
}

/* glm::vec3 PlayMode::get_leg_tip_position() {
	//the vertex position here was read from the model in blender:
	return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
} */