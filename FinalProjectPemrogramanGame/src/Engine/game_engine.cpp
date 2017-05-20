#include "Engine\game_engine.h"

void GameEngine::init(string title, unsigned int width, unsigned int height, bool vsync, WindowFlag windowFlag, unsigned int targetFrameRate, float timeScale) {
	//Initialize SDL
	SDL_Init(SDL_INIT_EVERYTHING);

	//Tell SDL that we want a double buffered window so we don't get any flickering
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	this->windowTitle = title;
	this->screenWidth = width;
	this->screenHeight = height;
	this->timeScale = timeScale;

	if (targetFrameRate > 0) {
		targetFrameTime = 1000.0f / targetFrameRate;
	}


	Uint32 flags = SDL_WINDOW_OPENGL;

	if (WindowFlag::EXCLUSIVE_FULLSCREEN == windowFlag) {
		flags |= SDL_WINDOW_FULLSCREEN;
	}

	if (WindowFlag::FULLSCREEN == windowFlag) {
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}

	if (WindowFlag::BORDERLESS == windowFlag) {
		flags |= SDL_WINDOW_BORDERLESS;
	}

	// Setup SDL Window
	window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, flags);
	if (window == nullptr) {
		logError("Failed to create SDL window!");
	}

	//Set up our OpenGL context
	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (glContext == nullptr) {
		logError("Failed to create SDL_GL context!");
	}

	//Set up glew (optional but recommended)
	GLenum glewStat = glewInit();
	if (glewStat != GLEW_OK) {
		logError("Failed to initialize glew!");
	}

	//Set VSYNC
	SDL_GL_SetSwapInterval(vsync);

	this->windowState = WindowState::RUNNING;
	// Init delta time calculation
	last = SDL_GetTicks();

	// Initialize everything
	this->input = new Input();

	// Initialize Shader
	Shader shaderSprite = ResourceManager::loadShader("sprite.vert", "sprite.frag", nullptr, "Sprite");

	box = GKV::Box(glm::vec2(100, 100), "wall.jpg", shaderSprite.id);
	box.position = glm::vec2(0, 0);
}

void GameEngine::cleanup() {
	// cleanup the all states
	while (!states.empty()) {
		states.back()->cleanup(this);
		states.pop_back();
	}

	delete(this->input);

	// shutdown SDL
	SDL_Quit();
}

void GameEngine::changeState(State * state) {
	// cleanup the current state
	if (!states.empty()) {
		states.back()->cleanup(this);
		states.pop_back();
	}

	// store and init the new state
	states.push_back(state);
	states.back()->init(this);
}

void GameEngine::pushState(State * state) {
	// pause current state
	if (!states.empty()) {
		states.back()->pause(this);
	}

	// store and init the new state
	states.push_back(state);
	states.back()->init(this);
}

void GameEngine::popState() {
	// cleanup the current state
	if (!states.empty()) {
		states.back()->cleanup(this);
		states.pop_back();
	}

	// resume previous state
	if (!states.empty()) {
		states.back()->resume(this);
	}
}

void GameEngine::updateFPS() {
	getFPS();
}

void GameEngine::handleEvent() {
	SDL_Event evt;

	//Will keep looping until there are no more events to process
	while (SDL_PollEvent(&evt)) {
		switch (evt.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			handleKeyEvent(evt.type, evt.key.keysym.sym);
		case SDL_QUIT:
			this->windowState = WindowState::EXIT;
			break;
		}
	}

	// Handle mouse position update
	int mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);
	mouseY = (float) this->screenHeight - mouseY;
	input->mousePos.x = mouseX;
	input->mousePos.y = mouseY;

	// let the state handle events
	if (!states.empty()) {
		states.back()->handleEvents(this);
	}
}

void GameEngine::update() {
	// let the state update the game
	if (!states.empty()) {
		states.back()->update(this);
	}
}

void GameEngine::draw() {
	glViewport(0, 0, screenWidth, screenHeight);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (this->mainCamera != nullptr) {
		Shader shader = ResourceManager::getShader("Sprite");
		shader.setMatrix4("view", this->mainCamera->getView(), true);
		shader.setMatrix4("projection", this->mainCamera->getProjection(), true);
	}

	box.render();


	glDisable(GL_BLEND);

	// let the state draw the screen
	if (!states.empty()) {
		states.back()->draw(this);
	}
}

void GameEngine::lateFPS() {
	SDL_GL_SwapWindow(window);
	limitFPS();

	if (debugFPS) {
		frameCounter++;
		if (frameCounter == 60) {
			string title = windowTitle + " FPS: " + std::to_string(fps);
			SDL_SetWindowTitle(window, title.c_str());
			frameCounter = 0;
		}
	}
}

void GameEngine::logError(string errorString) {
	std::cout << errorString << std::endl;
	SDL_Quit();
	exit(1);
}

void GameEngine::logDebug(string debugString) {
	std::cout << debugString << std::endl;
}

float GameEngine::getScreenHeight() {
	return screenHeight;
}

float GameEngine::getScreenWidth() {
	return screenWidth;
}

const Input * GameEngine::getInput() {
	return input;
}

glm::vec3 GameEngine::getMousePos() {
	return input->mousePos;
}

void GameEngine::setMainCamera(Camera * camera) {
	this->mainCamera = camera;
}

void GameEngine::unsetMainCamera() {
	this->mainCamera = nullptr;
}

void GameEngine::limitFPS() {
	//Limit the FPS to the max FPS
	SDL_Delay((Uint32)(targetFrameTime));
}

float GameEngine::getDeltaTime() {
	unsigned int time = SDL_GetTicks();
	unsigned int delta = time - lastFrame;
	lastFrame = time;

	return delta * timeScale;
}

void GameEngine::getFPS() {
	if (SDL_GetTicks() - last > 1000) {
		fps = _fps;
		_fps = 0;
		last += 1000;
	}
	_fps++;
}

void GameEngine::handleKeyEvent(Uint32 eventType, SDL_Keycode keycode) {
	if (eventType == SDL_KEYUP) {
		if (keycode == SDLK_ESCAPE) {
			windowState = WindowState::EXIT;
		}
	}
}