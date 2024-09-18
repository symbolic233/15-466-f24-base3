#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Scene::Transform *c4 = nullptr;
	Scene::Transform *cs4 = nullptr;

	float correct_timer = 0.0f;
	float correct_pause = 2.0f;
	bool paused = false;

	void set_answer();
	void clear_guess();
	void reset();
	static const uint32_t max_pressed = 4;
	uint32_t current_guesses = 0;
	uint32_t total_guesses = 0;
	uint32_t corrects = 0;

	uint32_t selection = 0;
	uint32_t answer = 0;
	void play_notes(uint32_t code);
	uint32_t note_count(uint32_t code);

	bool compare_guess = false;
	void submit_guess();

	bool sharp = false;
	static const uint32_t keycount = 12;
	std::string keys[keycount] = {"C4", "Cs4", "D4", "Ds4", "E4", "F4", "Fs4", "G4", "Gs4", "A4", "As4", "B4"};

	std::shared_ptr< Sound::PlayingSample > correct_play;
	std::shared_ptr< Sound::PlayingSample > incorrect_play;
	// vector initialization: https://stackoverflow.com/questions/29298026/stdvector-size-in-header

	typedef std::vector<std::shared_ptr< Sound::PlayingSample >> piano_store;
	piano_store piano_keys = std::vector<std::shared_ptr< Sound::PlayingSample >>(keycount);
	// piano_store piano_guess = std::vector<std::shared_ptr< Sound::PlayingSample >>(keycount);
	// piano_store piano_answer = std::vector<std::shared_ptr< Sound::PlayingSample >>(keycount);
	
	//camera:
	Scene::Camera *camera = nullptr;

};
