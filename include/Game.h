#pragma once

#include <vector>
#include <list>
#include <deque>
#include <random>
#include <memory>
#include <utility>
#include <boost/functional/hash.hpp>
#include <functional>
#include <boost/fiber/all.hpp>

#include "Constants.h"
#include "Tetrimino.h"
#include "Board.h"

typedef std::pair<board_t, double> boardAndScore_t;

struct CacheHash
{
    std::size_t operator()(const std::pair<Board, int>& key) const
    {
        boost::hash<std::pair<Board, int>> boardHash;
        return boardHash(key);
    }
};

class FindBestBoard_Rec_Channels;

class Game
{
    std::unique_ptr<std::mt19937> rng;
    std::unique_ptr<std::uniform_int_distribution<std::mt19937::result_type>> dist;
    std::deque<int> tetriminoQueue;
    std::unordered_map<std::pair<Board, int>, Board, CacheHash> bestBoardCache;
    
    //statistics stuff
    uint64_t deaths;
    uint64_t totalBlocks;
    double avgBlocksPerGame;
    double totalScore;
    Board board;

    void UpdateBoard(Board&& board);
    void ResetBoard();
    void CheckGameOver(const double& score);
    void UpdateQueue();
    void PrintFPS() const;
    void PrintStatistics() const;

    static std::vector<Tetrimino> LoadTetriminos();

public:
    static const std::vector<Tetrimino> tetriminos;

    Game();
    Board FindBestBoard_SingleThread() const;
    Board FindBestBoard_MultiThread() const;
    //ptr to functor
    std::unique_ptr<FindBestBoard_Rec_Channels> findBestBoard_Rec_Channels;
    
    void Update();
    void DebugContext(const Context&);
    
    static void Log(const Context&);
    static void Log(const Board&);
    static void Log(const std::string&);
    static void Fatal(const std::string&);
    static Context LoadContextFromFile(const std::string& fileName, const std::vector<Tetrimino>& tetriminos);
};

class FindBestBoard_Rec_Channels
{
    static const size_t chanSize = 512;
    FindBestBoard_Rec_Channels() = delete;
    boost::fibers::buffered_channel<std::tuple<Board, TetriminoRotation, std::deque<int>>> argsChan{chanSize};
    boost::fibers::buffered_channel<Board> resultChan{chanSize};
    std::vector<std::future<void>> workers;

public:
    uint numWorkers;
    FindBestBoard_Rec_Channels(const uint& workers);
    Board operator()(const Board& board, const std::deque<int>& tetriminoQueue);
};