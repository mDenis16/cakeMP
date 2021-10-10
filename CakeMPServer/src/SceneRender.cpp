#include <Common.h>

#include "GameServer.h"

#include <raylib.h>
#include <SceneRender.h>
#include <functional>
#include <iostream>

Camera camera = { 0 };

void SceneRender::Init()
{

	this->renderThread = new std::thread(std::bind(&SceneRender::OnRender, this));


}

void SceneRender::OnRender()
{
    //--------------------------------------------------------------------------------------
    
   
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "server view");
    /*m_position.x = -0.341730f;
    m_position.y = 525.319763f;
    m_position.z = 179.050201f;*/
    // Define the camera to look into our 3d world

    auto spawnPosition = Vector3{ -0.341730f, 525.319763f, 179.050201f };

    Camera camera = { 0 };
    camera.position = Vector3{ 4.0f, 2.0f, 4.0f };
    camera.target = Vector3{ 0.0f, 1.8f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Vector3 cubePosition = { 0.0f, 0.0f, 0.0f };

    SetCameraMode(camera, CAMERA_FIRST_PERSON); // Set a free camera mode
    UpdateCamera(&camera);          // Update camera
    SetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        if (_pServer->m_running && _pServer->m_network.m_hostListen) {



            UpdateCamera(&camera);          // Update camera

          
          
            BeginDrawing();

            ClearBackground(RAYWHITE);


            BeginMode3D(camera);
            
            DrawPlane(Vector3 { 0.0f, 0.0f, 0.0f }, Vector2 { 32.0f, 32.0f }, LIGHTGRAY); // Draw ground

            DrawGrid(10, 1.0f);

            SAFE_MODIFY(_pServer->m_network.m_players);

         
           
           
            for (auto player : _pServer->m_network.m_players) {

                auto pos = player->GetPosition();

              
                SAFE_MODIFY(player->lagRecords);

             
                DrawCylinder(Vector3{ pos.x - spawnPosition.x, 0, pos.y - spawnPosition.y }, 1.0f, 1.0f, 2.0f, 6, RED);
                if (!player->lagRecords.empty())
                {
	                for(auto record : player->lagRecords)
                    DrawCylinderWires(Vector3{ record->m_position.x - spawnPosition.x, 0, record->m_position.y - spawnPosition.y }, 1.0f, 1.0f, 2.0f, 6, Color{ 0,0, 200, 150 });
                }



            }
            EndMode3D();
            EndDrawing();
        }
     
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseRayWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

}