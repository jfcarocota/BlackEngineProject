#include<iostream>
#include "Game.hh"
#include <mach-o/dyld.h>
#include <unistd.h>
#include <libgen.h>

int main()
{
    // Change working directory to the executable's location on macOS
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        char* dir = dirname(path);
        chdir(dir);
    }

    Game* game{new Game()};
    game->Initialize();

    delete game;

    return EXIT_SUCCESS;
}
