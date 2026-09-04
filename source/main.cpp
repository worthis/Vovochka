// source/main.cpp
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

    vovochka::Game game(dataRoot);
    if (!game.init(1280, 720, "Vovochka"))
    {
        std::fprintf(stderr, "Game init failed\n");
        return 1;
    }
    game.run();
    return 0;
}