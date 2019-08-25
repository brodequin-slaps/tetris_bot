#pragma once

#include <array>
#include <string>

#include "Constants.h" 
#include "Tetrimino.h"

class Board;

typedef std::array<WidthInt, BLOCKS_H> board_t;
typedef std::pair<Board, Tetrimino> Context;

class Board
{    
public:
    double score;
    board_t boardArr;

    Board();
    void Reset();
    void SetScore(const int& destroyedLines, const int& dropHeight, const int& tetriminoRotationHeight);
    double CalculateScore(const int& destroyedLines, const int& dropHeight, const int& tetriminoRotationHeight) const;
    int DropTetriminoRotation(const TetriminoRotation& tr);
    int DestroyLines(const int& dropHeight, const int& trHeight);

    //drops the tetrimino and invoke scopeCalculator, which will modify the best value
    //scopeCalculator must be a lambda which captures the best value to modify
    template <typename ScoreCalculator>
    void DropAndUpdateScore(const TetriminoRotation& tr, ScoreCalculator&& scoreCalculator)
    {
        int dropHeight = DropTetriminoRotation(tr);
        if(dropHeight >= 0)
        {
            int destroyedLines = DestroyLines(dropHeight, tr.height);
            scoreCalculator(destroyedLines, dropHeight, tr);
        }
    }

    //top level score calculator
    void ResursiveScoreCalculator(const int& destroyedLines, const int& dropHeight, const TetriminoRotation& tr, Board& best, const std::deque<int>& tetriminoQueue) const;
    //nested score calculator called if recursion level > 1
    void SubScoreCalculator(const int& destroyedLines, const int& dropHeight, const TetriminoRotation& tr, double& best, const int& currentDepth, const int& maxDepth, const std::deque<int>& tetriminoQueue) const;
    double BestSubScore(const int& currentDepth, const int& maxDepth, const std::deque<int>& tetriminoQueue) const;

    std::string Serialize() const;
    void Print(int spaces = 10) const;

    static Board Deserialize(std::string filename);

    friend bool operator==(const Board& lhs, const Board& rhs);
    friend std::size_t hash_value(const Board& board);
};