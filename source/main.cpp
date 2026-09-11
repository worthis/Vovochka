#include "MenuSystem.h"
#include "Game.h"
#include <cstdio>

int main(int argc, char **argv)
{
    if (!std::freopen("game_log.txt", "w", stdout))
    {
    }
    if (!std::freopen("game_log.txt", "a", stderr))
    {
    }

    // data/ рядом с бинарником либо через argv[1]
    std::string dataRoot = (argc > 1) ? argv[1] : "data";

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Vovochka");
    InitAudioDevice();
    SetTargetFPS(60);

    ConfigSystem::instance().loadSettings("settings.json");

    vovochka::MenuSystem menu(dataRoot);
    menu.init();

    vovochka::InputSystem input;
    vovochka::Game game(dataRoot);
    bool gameInitialized = false;

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();
        input.update();

        if (menu.currentScreen() != vovochka::MenuScreen::InGame)
        {
            menu.update(dt, input);
            menu.render();

            if (menu.isQuitRequested())
                break;

            if (menu.isStartGameRequested())
            {
                if (!gameInitialized)
                {
                    game.init();
                    gameInitialized = true;
                }
                game.setLevel(menu.selectedDifficulty());
                menu.resetRequests();
            }
        }
        else
        {
            if (!gameInitialized)
            {

                BeginDrawing();
                ClearBackground(BLACK);
                EndDrawing();

                continue;
            }

            game.update(dt);
            game.render();

            if (!game.isRunning())
                menu.setScreen(vovochka::MenuScreen::GameOver);
        }
    }

    menu.shutdown();
    if (gameInitialized)
        game.shutdown();

    CloseAudioDevice();
    CloseWindow();
    return 0;
}