#include <iostream>
#include <math.h>
#include <chrono>
#include <string>

#include "Game.h"

using namespace std;

int main()
{
    Game game;

    for(;;)
    {
        game.Update();
    }

    return 0;
}