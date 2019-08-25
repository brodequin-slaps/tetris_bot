#pragma once

#include <cstdint>
#include <math.h>
#include <string>

#define TETRIMINO_DIR "../tetriminos/"
const std::string LOGS_DIR = "../logs/";
const std::string BOARDS_LOG_DIR = LOGS_DIR + "boards/";
const std::string CONTEXTS_LOG_DIR = LOGS_DIR + "contexts/";

typedef std::uint32_t WidthInt;
const int MAX_WIDTH = 32; 
const int NUM_PIECES = 7;
const int BLOCKS_W = 10;
const int BLOCKS_H = 20;
const int LOOK_AHEAD = 1; 

const WidthInt FULL_LINE = (~(WidthInt(0))) ^ (WidthInt)(pow(2, MAX_WIDTH - BLOCKS_W) - 1);