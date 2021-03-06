#include <Managers/GameManager.h>
#include <Managers/LiveReloadManager.h>

#include <filesystem>
#include <Core/Entity.h>

// Components to hook up with event manager
#include <Components/TransformComponent.h>
#include <Components/RenderableComponent.h>
#include <Components/AnimationComponent.h>
#include <Components/AudioSourceComponent.h>
#include <Components/AudioListenerComponent.h>
#include <Components/AudioGeometryComponent.h>
#include <Components/ScriptComponent.h>
#include <Components/CameraComponent.h>
#include <Components/EmissionComponent.h>

// GUI
#include <EngineGUI/AudioPanel.h>
#include <EngineGUI/AnimationPanel.h>
#include <EngineGUI/ScenePanel.h>
#include <EngineGUI/PresetObjectPanel.h>
#include <EngineGUI/HierarchyPanel.h>
#include <EngineGUI/InspectorPanel.h>
#include <EngineGUI/ImGuizmoTool.h>
#include <EngineGUI/SkyboxPanel.h>
#include <EngineGUI/FullscreenShaderPanel.h>

#include <FullscreenShader.h>

using NoxEngineUtils::Logger;
using NoxEngine::EventManager;
using NoxEngine::Entity;

using namespace NoxEngine;
using namespace NoxEngineGUI;

GameManager::GameManager() :
	title(WINDOW_TITLE),
	ui_params(),
	should_close(false),
	keys(),
	game_state()
{
	game_state.win_width = WINDOW_WIDTH;
	game_state.win_height = WINDOW_HEIGHT;
}

void GameManager::init() {
	LOG_DEBUG("Initing systems");
	init_window();
	init_scene();
	init_ecs();
	init_events();
	init_audio();
	init_camera();
	init_shaders();
	init_gui();
	init_animation();
	init_renderer();
	//init_scripts();
	init_postprocess();
}

void GameManager::update() {

	update_livereloads();
	update_inputs();
	update_ecs();
	update_animation();
	update_audio();
	update_renderer();
	update_postprocessors();
	update_gui();

	// Support closing via close button again
	if (glfwWindowShouldClose(window)) {
		should_close = true;
	}

	glfwSwapBuffers(window);
}

// Whenever an entity is created, modified, or deleted, call this function
// This is done to simplify code since previously we had renderer->addObject() everywhere
void GameManager::scheduleUpdateECS() {
	updateNeededECS = true;
}

// Create audio geometry from the entity's AudioGeometryComponent and add it to the renderer
void GameManager::createAudioGeometry(Entity* ent, IAudioGeometry* igeo) {

	// TODO: First remove this object from audio engine & renderer first
	// ...
	// audioManager->removeGeometry(igeo->geometryId);
	// renderer->removeObject(igeo->rendObjId);

	// Create one Geometry object with all meshes in the meshScene
	igeo->geometryId = audioManager->createGeometry(ent);

	// Add to renderer
	renderer->addObject(ent, igeo, ComponentType::AudioGeometryType);

	// Get the MeshScene reference (if it has one - creating plane / bounding box will not generate one), 
	// set the rendObjId of meshes to be the newly added object in the renderer
	if (igeo->meshScene) {
		for (u32 i = 0; i < igeo->meshScene->meshes.size(); i++) {
			igeo->meshScene->meshes[i]->rendObjId = igeo->rendObjId;
		}
	}

	// Update renderer
	// TODO-OPTIMIZATION: Set a flag inside GameManager, update buffers in a batch fashion
	this->renderer->updateBuffers();
}


void callback(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam) {

	LOG_DEBUG("Message: %s", message);
}

void GameManager::init_window() {
	LOG_DEBUG("Initializing Window");

	if (!glfwInit()) {
		LOG_DEBUG("Error initializing glfw...exiting.");
		exit(1);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_SAMPLES, 4);

	window = glfwCreateWindow(game_state.win_width, game_state.win_height, title.c_str(), nullptr, nullptr);

	glfwMakeContextCurrent(window);
	glfwSetWindowPos(window, 100, 100);

	if (window == nullptr) {
		LOG_DEBUG("Failed to create window");
		exit(1);
	}

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		LOG_DEBUG("Failed to initialize OpenGL context");
	}

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
	glDebugMessageCallback(callback, NULL);

	glfwSetWindowUserPointer(window, this);

	auto keysFunc = [](GLFWwindow *w, i32 key, i32 scan, i32 action, i32 mods){
		glfwGetWindowUserPointer(w);

		GameManager *gm = (GameManager *)glfwGetWindowUserPointer(w);
		if(key >= GLFW_KEY_SPACE && key < GLFW_KEY_ESCAPE) {
			if(action == GLFW_PRESS) gm->keys[(char)(key)] = 1;
			if(action == GLFW_RELEASE) gm->keys[(char)(key)] = 0;
		}

		if(key == GLFW_KEY_ESCAPE) gm->should_close = true;

	};

	auto mouseKeysFunc = [](GLFWwindow *w, i32 key, i32 action, i32 mods){
		glfwGetWindowUserPointer(w);

		GameManager *gm = (GameManager *)glfwGetWindowUserPointer(w);
		if(key == GLFW_MOUSE_BUTTON_LEFT) {
			if(action == GLFW_PRESS) gm->game_state.mouse_left = 1;
			else if(action == GLFW_RELEASE) gm->game_state.mouse_left = 0;
		}

		if(key == GLFW_MOUSE_BUTTON_RIGHT) {
			if(action == GLFW_PRESS) gm->game_state.mouse_right = 1;
			else if(action == GLFW_RELEASE) gm->game_state.mouse_right = 0;
		}
	};


	glfwSetKeyCallback(window, keysFunc);
	glfwSetMouseButtonCallback(window, mouseKeysFunc);
}

// Similar to activateScene
void GameManager::init_ecs() {

	initComponentTypes();

	update_ecs();
}


void GameManager::init_events() {

	EventManager::Instance()->addListener(EventNames::meshAdded, [this](va_list args){

			// Steven: That's how I would do it
			// clean up: leaky mem
			String file_name = va_arg(args, char*);

			// Add to hash map if it does not exist
			if (game_state.meshScenes.find(file_name) == game_state.meshScenes.end()) {
				game_state.meshScenes.emplace(file_name, NoxEngine::readFBX(file_name.c_str()));
			}
			MeshScene& meshScene = game_state.meshScenes.find(file_name)->second;

			i32 index = game_state.activeScene->entities.size();
			// We're treating every mesh as an entity FOR NOW
			for (u32 i = 0; i < meshScene.meshes.size(); i++)
			{
				// Note (Vincent): this is more or less the same as letting the scene automatically allocate an entity,
				//                 because the entity ID is managed by the scene
				Entity* ent = new Entity(game_state.activeScene, meshScene.meshes[i]->name.c_str(),
					file_name.c_str());

				Mesh* mesh = new Mesh(*meshScene.meshes[i]);
				RenderableComponent* comp = mesh;
				TransformComponent* trans = new TransformComponent(0.0, 0.0, 0.0);

				// ScriptComponent *script = new ScriptComponent("assets/scripts/test.lua");

				ent->addComp(comp);
				ent->addComp(trans);
				// ent->addComp(script);

				if (meshScene.hasAnimations())
				{
					// Get the node associated to this mesh
					for (MeshNode* node : meshScene.allNodes)
					{
						if (node->hasAnimations() && node->meshIndex[0] == i)
						{
							AnimationComponent* anim = new AnimationComponent(meshScene, node);
							ent->addComp(anim);
							break;
						}
					}
				}

				game_state.activeScene->addEntity(ent);
			}
	});


	EventManager::Instance()->addListener(EventNames::componentAdded, [this](va_list args) {

			Entity* ent = va_arg(args, Entity*);
			const std::type_index compTypeId = va_arg(args, std::type_index);

			// Renderer
			if (ent->containsComps<RenderableComponent>()) {

				RenderableComponent* rendComp = ent->getComp<RenderableComponent>();

				if (!renderer->hasRendObj(rendComp->rendObjId)) {

					renderer->addObject(ent, rendComp, ComponentType::RenderableType);

					this->renderer->updateBuffers();
				}
			}

			// Light/Emission
			if (ent->containsComps<EmissionComponent>())
			{
				renderer->addLights(ent);
			}

			// Audio
			// Geometry: 
			//		We don't add the mesh to the audio engine when the geometry component is added, 
			//		because it's expected to be empty. We add to audio engine when mesh is loaded
			// AudioSource: 
			//		No need to do anything special
			// AudioListener: 
			//		First listener is set to active;
			//		Add fake mesh to renderer
			if (compTypeId == typeid(AudioListenerComponent)) {

				AudioListenerComponent* lisComp = ent->getComp<AudioListenerComponent>();

				// First listener
				if (game_state.activeAudioListener == nullptr) game_state.activeAudioListener = ent;

				// Add mesh to renderer
				if (!renderer->hasRendObj(lisComp->rendObjId)) {
					
					renderer->addObject(ent, lisComp, ComponentType::AudioListenerType);

					this->renderer->updateBuffers();
				}
			}

			// ...
	});


	EventManager::Instance()->addListener(EventNames::componentRemoved, [this](va_list args) {

			Entity* ent = va_arg(args, Entity*);
			const std::type_index compTypeId = va_arg(args, std::type_index);

			if (compTypeId == typeid(RenderableComponent)) {
				renderer->removeObject(ent->getComp<RenderableComponent>()->rendObjId);
				// TODO-OPTIMIZATION: Remove in batches every X ms, shift the still-valid indices to take the free space
			}

			// Audio
			if (compTypeId == typeid(AudioGeometryComponent)) {

				AudioGeometryComponent* geoComp = ent->getComp<AudioGeometryComponent>();
				
				// Disable the loaded geometry
				// TODO: remove it
				audioManager->setGeometryActive(geoComp->geometryId, false);
				renderer->removeObject(geoComp->rendObjId);
			}

			if (compTypeId == typeid(AudioListenerComponent)) {

				AudioListenerComponent* lisComp = ent->getComp<AudioListenerComponent>();
				
				// Remove the fake mesh from the renderer
				renderer->removeObject(lisComp->rendObjId);

				// TODO: Invalidate listener
			}

			// ...
	});
}

void GameManager::init_audio() {
	audioManager = AudioManager::Instance();
	audioManager->init();

	audioManager->set3dListenerAttributes(
			{ 0.0f, 0.0f, 0.0f },		// Position
			{ 0.0f, 0.0f, 0.0f },		// velocity (TODO: calculate)
			{ 0.0f, 0.0f, -1.0f },		// Forward
			{ 0.0f, 1.0f, 0.0f }		// Up
			);

	game_state.activeAudioListener = nullptr;
}

void GameManager::init_camera() {

	game_state.cameras.push_back(new Camera(vec3(100.0f, 100.0f, 100.0f), vec3(0.0f, 0.0f, 0.0f)));
	game_state.cameras.push_back(new Camera(vec3(100.0f, 10.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f)));
	game_state.cameras.push_back(new Camera(vec3(20.0f, 0.0f, 20.0f), vec3(0.0f, 0.0f, 0.0f)));
}

void GameManager::init_shaders() {

	programs.emplace_back(Array<ShaderFile>{
		{ "assets/shaders/vShader.glsl", GL_VERTEX_SHADER, 0 },
		{ "assets/shaders/fShader.glsl", GL_FRAGMENT_SHADER, 0 },
	});

	programs.push_back(Array<ShaderFile>{
		{"assets/shaders/vertexShader.vs", GL_VERTEX_SHADER, 0},
		{ "assets/shaders/fragmentShader.fs", GL_FRAGMENT_SHADER, 0 }
	});

	current_program = &programs[0];
}

void GameManager::init_animation() {
	currentTime = glfwGetTime();
	deltaTime = 0;
	lastTime = currentTime;
}

void GameManager::init_renderer() { 
	renderer = new Renderer(game_state.win_width, game_state.win_height, game_state.cameras[0]);
	renderer->setCamera(game_state.cameras[0]);
	renderer->setProgram(current_program);
	renderer->updateProgram();

	game_state.renderer = renderer;
	renderer->setFrameBufferToTexture();
	game_state.fullscreen_shader_texture_used = renderer->getTexture();

	GridObject *obj = new GridObject(vec3(-1500, 0, -1500), vec3(1500, 0, 1500), 150);
	renderer->addPermObject(obj);
	renderer->updateBuffers();

	// MULTIPLE LIGHTS Init lights. Will be removed when light will be added dinamically
	//for (u32 i = 0; i < 3; i++)
	//{
	//	game_state.lightSources.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
	//	renderer->addLights(game_state.lightSources[i]);
	//}

	//renderer->_change_num_of_light(3);
	//renderer->updateProgram();
}

void GameManager::init_gui() {

	NoxEngineGUI::init_imgui(window);
	
	ImGuiIO& io = ImGui::GetIO();
	font = io.Fonts->AddFontFromFileTTF("assets/font/envy.ttf", 18);
	io.Fonts->Build();

	// Initialize panel variables
	NoxEngineGUI::initPresetObjectPanel();

	// Initialize gui params
	ui_params.selectedEntity = -1;
	ui_params.sceneBackgroundColor = 0x282828FF;
}

void GameManager::init_scene() {
	game_state.scenes.push_back(new Scene());
	game_state.activeScene = game_state.scenes[0];
}

void NoxEngine::GameManager::init_scripts() {
	
}

void GameManager::init_postprocess() { 

	i32 indices[6] = { 0, 2, 1, 0, 3, 2 };

	glGenVertexArrays(1, &post_process_vao);
	glBindVertexArray(post_process_vao);
	glGenBuffers(1, &post_process_quad_index);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, post_process_quad_index);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(i32)*6, indices, GL_STATIC_DRAW);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void GameManager::update_livereloads() {
	LiveReloadManager *lrManager = LiveReloadManager::Instance();
	lrManager->checkFiles();
}

void GameManager::update_inputs() {
	glfwPollEvents();

	if(keys['W']) { renderer->getCamera()->moveFwdBy(0.9f); }
	if(keys['S']) { renderer->getCamera()->moveFwdBy(-0.9f); }
	if(keys['D']) { renderer->getCamera()->moveHorBy(0.9f); }
	if(keys['A']) { renderer->getCamera()->moveHorBy(-0.9f); }
	if(keys[' ']) { renderer->getCamera()->moveVerBy(0.9f); }
	if(keys['K']) { renderer->getCamera()->moveVerBy(-0.9f); }

	if(keys['1']) { renderer->setCamera(game_state.cameras[0]); }
	if(keys['2']) { renderer->setCamera(game_state.cameras[1]); }
	if(keys['3']) { renderer->setCamera(game_state.cameras[2]); }

	f64 x;
	f64 y;

	glfwGetCursorPos(window, &x, &y);
	game_state.mouse_x_delta = game_state.mouse_x - x;
	game_state.mouse_y_delta = game_state.mouse_y - y;

	game_state.mouse_x = x;
	game_state.mouse_y = y;

	// full_screen is set to false when ScenePanel is focused
	if(game_state.mouse_right && (ui_params.scene_active)) {
		renderer->getCamera()->moveToMousePos(
			game_state.mouse_x_delta*deltaTime,
			game_state.mouse_y_delta*deltaTime
		);

		ui_params.scene_active = false;
	}

	if (keys['Z']) { ui_params.imguizmoMode = ImGuizmo::OPERATION::TRANSLATE; };
	if (keys['X']) { ui_params.imguizmoMode = ImGuizmo::OPERATION::ROTATE; };
	if (keys['C']) { ui_params.imguizmoMode = ImGuizmo::OPERATION::SCALE; };

}

void GameManager::update_ecs() {

	size_t nEntities = game_state.activeScene->entities.size();

	const auto entities = game_state.activeScene->entities;
	for (i32 i = 0; i < nEntities; i++)
	{
		// script update
		entities[i]->tick(deltaTime, currentTime);

		// synchronize audio listener's active state
		AudioListenerComponent* lisComp = entities[i]->getComp<AudioListenerComponent>();
		if (lisComp) lisComp->active = (game_state.activeAudioListener == entities[i]);
	}

	if (!updateNeededECS) return;

	bool entityRemoved = false;

	// Check for entity removal. 
	// Free resources explicitly (since `entities` contain raw `Entity*` pointers) and resize vector
	game_state.activeScene->entities.erase(
		std::remove_if(
			game_state.activeScene->entities.begin(),
			game_state.activeScene->entities.end(),
			[](auto&& ent) {
				bool rm = ent->remove;
				if (rm) delete ent;
				return rm;
			}
		),
		game_state.activeScene->entities.end()
	);

	entityRemoved = nEntities != game_state.activeScene->entities.size();

	// update subsystems if needed
	if (/*updateRenderer && */entityRemoved) renderer->updateBuffers();
	//if (updateAudioManager) audioManager->...

	// Update done
	updateNeededECS = false;
}

void GameManager::update_gui() {

	ImGuiIO& io = ImGui::GetIO();
	
	char windowTitle[64];
	snprintf(windowTitle, 64, "%s - FPS %.3f", title.c_str(), io.Framerate);
	glfwSetWindowTitle(window, windowTitle);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));		// Vincnet: pushing no padding style then drawing all panels causes all panels to lose padding.
	
	ImGui::PushFont(font);

	NoxEngineGUI::updateGUI(game_state, &ui_params);

	if(!ui_params.full_screen) {
		// NoxEngineGUI::updateAudioPanel(&game_state, &ui_params);
		NoxEngineGUI::updateAnimationPanel(&game_state, &ui_params);
		NoxEngineGUI::updateHierarchyPanel(&game_state, &ui_params);
		NoxEngineGUI::updateSkyboxPanel(&game_state);
		NoxEngineGUI::updateInspectorPanel(&game_state, &ui_params);
	} else {
		NoxEngineGUI::updatePostProcessorsPanel(&game_state, &ui_params);
	}

	NoxEngineGUI::updatePresetObjectPanel(&game_state);
	NoxEngineGUI::updateScenePanel(&game_state, &ui_params);
	NoxEngineGUI::updateFullscreenShaderPanel(&game_state, &ui_params);
	
	// Make sure the scene panel is focused when we run
	if(ui_params.firstLoop) {
		ImGui::SetWindowFocus(kPanelNameMap[ PanelName::Scene ].c_str());
		ui_params.firstLoop = false;
		ui_params.full_screen = false;
	}


	ImGui::PopFont();
	//ImGui::PopStyleVar();
	ImGui::Render();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void GameManager::update_audio() {

	ITransform*		itrans	= nullptr;
	IAudioListener* ilisten = nullptr;
	IAudioSource*	isrc	= nullptr;
	IAudioGeometry* igeo	= nullptr;

	// Set listener attributes
	{
		vec3 pos{ 0 };
		vec3 vel{ 0 };
		vec3 forward(0.0f, 0.0f, 1.0f);
		vec3 up(0.0f, 1.0f, 0.0f);

		if (game_state.activeAudioListener) {
			itrans	= game_state.activeAudioListener->getComp<TransformComponent>();
			ilisten = game_state.activeAudioListener->getComp<AudioListenerComponent>();

			if (itrans) {
				mat4 rotation = glm::eulerAngleXYZ(itrans->rx, itrans->ry, itrans->rz);

				pos		= vec3(itrans->x, itrans->y, itrans->z);
				forward = rotation * vec4(forward, 1.0f);
				up		= rotation * vec4(up, 1.0f);
			}

			if (ilisten) vel = ilisten->vVel;
		}

		//audioManager->set3dListenerAttributes(itrans);
		audioManager->set3dListenerAttributes(pos, vel, forward, up);
	}

	// Update geometry states & orientation
	for (Entity* ent : game_state.activeScene->entities) {

		if (!ent->isEntityEnabled()) continue;

		itrans	= ent->getComp<TransformComponent>();
		isrc	= ent->getComp<AudioSourceComponent>();
		igeo	= ent->getComp<AudioGeometryComponent>();

		// TODO: fix handedness
		vec3 pos( 0.0f );
		vec3 forward( 0.0f, 0.0f, 1.0f );
		vec3 up( 0.0f, 1.0f, 0.0f );
		vec3 scale( 1.0f );

		if (itrans && ent->isEnabled<TransformComponent>()) {

			mat4 rotation = glm::eulerAngleXYZ(itrans->rx, itrans->ry, itrans->rz);

			pos		= vec3(itrans->x, itrans->y, itrans->z);
			forward = rotation * vec4(forward, 1.0f);
			up		= rotation * vec4(up, 1.0f);
			scale	= vec3(itrans->sx, itrans->sy, itrans->sz);
		}

		if (isrc && !isrc->stopped) {

			// TODO: stop audio if it's disabled
			//if (!ent->isEnabled<AudioSourceComponent>()) audioManager->stopSound(0);

			audioManager->setChannel3dPosition(isrc->channelId, pos);
			audioManager->setChannelVolume(isrc->channelId, isrc->volume);
		}

		if (igeo) {

			// Geometry component does not have a valid mesh (e.g. not yet loaded), don't do anything
			if (igeo->geometryId == -1) continue;

			// Set active
			audioManager->setGeometryActive(igeo->geometryId, ent->isEnabled<AudioGeometryComponent>());

			// Orient geometry
			if (ent->containsComps<TransformComponent>() && ent->isEnabled<TransformComponent>()) {

				audioManager->orientGeometry(igeo->geometryId, pos, forward, up, scale);
			}
		}
	}

	audioManager->update();
}


void GameManager::update_animation() {

	currentTime = glfwGetTime();
	deltaTime = currentTime - lastTime;
	lastTime = currentTime;

	for (Entity* ent : game_state.activeScene->entities) { 

		AnimationComponent *animComp = ent->getComp<AnimationComponent>();

		if (animComp) {
			animComp->update(deltaTime);
		}
	}
}

void GameManager::update_renderer() {

	if(game_state.prev_win_width != game_state.win_width ||
	   game_state.prev_win_height != game_state.win_height)
	{
		renderer->updateTextureSizes(game_state.win_width, game_state.win_height);
	}

	for(u32 i = 0; i < game_state.post_processors.size(); i++) {
		if(game_state.prev_win_width != game_state.win_width ||
			game_state.prev_win_height != game_state.win_height)
		{
			game_state.post_processors[i].changeTextureSize(game_state.win_width, game_state.win_height);
		}
	}


	for (Entity* ent : game_state.activeScene->entities) {
		if (ent->containsComps<RenderableComponent, AnimationComponent>())
		{
			RenderableComponent *rendComp = ent->getComp<RenderableComponent>();
			AnimationComponent* animComp = ent->getComp<AnimationComponent>();
			mat4 transformation = animComp->getTransformation();
			renderer->updateObjectTransformation(transformation, rendComp->rendObjId);
		}
	}

	for (u32 i = 0; i < renderer->getNumLights(); i++)
	{
		renderer->updateLightPos(i);
	}

	renderer->setFrameBufferToTexture();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	renderer->updateCamera();

	renderer->setProgram(&programs[1]);
	// maybe have a sun? 
	renderer->drawSkyBox();

	renderer->setProgram(current_program);

	// Update lights
	//for(int i = 0; i < game_state.lightSources.size(); i++)
	//	renderer->updateLightPos(i, game_state.lightSources[i][0], game_state.lightSources[i][1], game_state.lightSources[i][2]);
	renderer->fillBackground(ui_params.sceneBackgroundColor);
	renderer->draw();

	renderer->setFrameBufferToDefault();
}


void GameManager::update_postprocessors() {
	if(ui_params.full_screen) {
		glBindVertexArray(post_process_vao);
		for(u32 i = 0; i < game_state.post_processors.size(); i++) {
			if(game_state.post_processors[i].IsInit())
				game_state.post_processors[i].draw(currentTime);
		}
	}
}

////////////////////////////
/*   Different managers   */
////////////////////////////

bool GameManager::loadAudioSource(Entity* ent, IAudioSource *isrc) {

	String file_name = IOManager::Instance()->PickFile("All Files\0*.*\0\0");
	if (file_name.length() <= 0) return false;

	isrc->filePath = file_name;

	audioManager->loadSound(isrc);
	isrc->loaded = true;

	return true;
}

bool GameManager::loadAudioGeometry(Entity* ent, IAudioGeometry *igeo) {

	String file_name = IOManager::Instance()->PickFile("All Files\0*.*\0\0");
	if (file_name.length() <= 0) return false;

	game_state.meshScenes.emplace(file_name, NoxEngine::readFBX(file_name.c_str()));
	MeshScene& meshScene = game_state.meshScenes.find(file_name)->second;

	// Convert meshScene into vertices, face indices for Geometry triangles, and indices for renderer lines
	return igeo->loadMesh(&meshScene);
}

bool GameManager::playSound(Entity* ent, IAudioSource* isrc) {

	ITransform* itrans = ent->getComp<TransformComponent>();
	vec3 pos{ itrans->x, itrans->y, itrans->z };

	audioManager->loadSound(isrc);
	isrc->channelId = audioManager->playSounds(isrc, pos);

	return true;
}

bool GameManager::pauseUnpauseSound(Entity* ent, IAudioSource* isrc) {
	return audioManager->pauseUnpauseSound(isrc);
}

bool GameManager::stopSound(Entity* ent, IAudioSource* isrc) {
	bool success = audioManager->stopSound(isrc);
	if (success) {
		isrc->channelId = -1;
		isrc->paused = false;
		isrc->stopped = false;
	}

	return success;
}

int GameManager::createDSP(DSP_TYPE type) {

	return audioManager->createDSP(type);
}

//int GameManager::destroyDSP(int dspId) {}

DSP* GameManager::getDSP(int dspId) {
	return audioManager->getDSP(dspId);
}
