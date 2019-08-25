#pragma once

#include <string>
#include <vector>
#include "Constants.h"


class TetriminoRotation{
public:
    std::vector<WidthInt> piece;
    int width; 
    int height;
};


class Tetrimino{
    Tetrimino() = delete;
public:
    std::string name;
    std::vector<TetriminoRotation> tetriminoRotations;

    Tetrimino(const std::string&, int);
};