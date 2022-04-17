#include "EngineGUI/AnimationPanel.h"

// hacky solution to min and max being redefined in Windows.h, causing std::min/std::max
// to give errors. See https://stackoverflow.com/questions/2561368/illegal-token-on-right-side-of
#include <limits>
#define NOMINMAX

#include <Windows.h>
#include <FBXFileLoader.h>
#include <EventManager.h>

#include <glm/gtx/string_cast.hpp>

using namespace NoxEngine;

void NoxEngineGUI::updateAnimationPanel(NoxEngine::GameState *game_state) {

	// Variables
	std::string name = PANEL_NAME_MAP[ PanelName::AnimationSettings ];
	bool isAlreadyLoaded = false;

	// Window Begin
	ImGui::Begin(name.c_str());


	if (ImGui::Button("Load FBX File")) {

		String picked_file = IOManager::Instance()->PickFile("All Files\0*.*\0\0");

		if (picked_file.length() > 0)
		{
			const aiScene* pScene = NoxEngine::readFBX(picked_file.c_str());
			if (pScene != nullptr) {
				game_state->meshes.emplace(picked_file, pScene);
				
				// game_state->renderer->addObject(mesh);
				// game_state->renderer->updateBuffers();
				EventManager::Instance()->signal("mesh_added", picked_file, game_state->meshes.rbegin()->second);
			}
			else
			{
				printf("Error: converting sence to mesh data");
			}
		}
		else
		{
			printf("Error: reading fbx file");
		}
	}

	auto startItr = game_state->meshes.begin();
	auto endItr = game_state->meshes.end();

	while (startItr != endItr)
	{
		if (startItr->second.getNumOfAnimations() == 0)
		{
			ImGui::Text(startItr->second.nodeHierarchy.name.c_str());
			startItr++;
			continue;
		}

		if (ImGui::TreeNode(startItr->second.nodeHierarchy.name.c_str()))
		{
			if (ImGui::BeginTable("Animation 0 data: Node 0", 2, 
				ImGuiTableFlags_Borders|ImGuiTableFlags_Resizable))
			{
				for (i32 row = 0; row <
					startItr->second.nodeAnimTransformation[0][0].size(); row++)
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("Frame %d", row);
					ImGui::TableNextColumn();

					std::string someString
						= glm::to_string(startItr->second.nodeAnimTransformation[0][0][row]);

					const char* displayText = someString.c_str();

					ImGui::Text(displayText);
				}
				ImGui::EndTable();
			}
			ImGui::TreePop();
		}
		
		startItr++;
	}


	// If there was a mesh loaded by user
	// TODO: Might be a good idea to move this to ScenePanel?
	//       Have a queue of loaded meshes then add/update in ScenePanel if it's not empty
	// if (isAlreadyLoaded) {
	// 	mesh->prepForRenderer();
	// 	game_state->renderer->addObject(mesh);
	// 	game_state->renderer->updateBuffers();
	// 	delete mesh;
	// }


	// Window End
	ImGui::End();
}
