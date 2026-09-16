#include "MenuSystem.h"
#include "ConfigSystem.h"
#include "SaveSystem.h"
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

    srand(time(NULL));

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Vovochka");
    InitAudioDevice();
    SetTargetFPS(60);
    DisableCursor();
    SetExitKey(KEY_NULL);

    ConfigSystem::instance().loadSettings("settings.json");
    vovochka::SaveSystem::instance().load("save.json");
    vovochka::SaveSystem::instance().loadVideoDurations(dataRoot);

    vovochka::InputSystem input;
    vovochka::MenuSystem menu(dataRoot);
    vovochka::Game game(dataRoot);
    menu.init();

    bool gameInitialized = false;

    while (!WindowShouldClose())
    {
        const float dt = GetFrameTime();

        input.update(dt);

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
                game.setLevel(1, menu.selectedDifficulty(), true);
                menu.resetRequests();
            }

            if (menu.isResumeGameRequested())
            {
                game.proceedToNextLevel();
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

            if (input.isPausePressed())
            {
                menu.enterPause();
                continue;
            }

            game.update(dt, input);
            game.render();

            if (game.isLevelCompleted())
            {
                if (!menu.openLevelVideo(game.completedLevel()))
                    game.proceedToNextLevel();
            }
            else if (!game.isRunning())
            {
                menu.goGameOver(false);
            }
        }
    }

    menu.shutdown();
    if (gameInitialized)
        game.shutdown();

    CloseAudioDevice();
    CloseWindow();
    return 0;
}