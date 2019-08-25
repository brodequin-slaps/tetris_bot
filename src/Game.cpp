#include <stdexcept>
#include <random>
#include <iostream>
#include <functional>
#include <queue>
#include <fstream>
#include <sstream>

#include "Helpers.h"
#include "Game.h"
#include "utility"
#include "Board.h"

using namespace std;

void Game::Update()
{
	UpdateQueue();

	auto&& bestboard = (*findBestBoard_Rec_Channels)(board, tetriminoQueue);
	//auto&& bestboard = FindBestBoard_SingleThread();

	UpdateBoard(move(bestboard));
	totalBlocks++;

	CheckGameOver(board.score);
	PrintFPS();

	//PrintBoard();
	//cout << "Press Enter to Continue";
	//cin.ignore();
}

void Game::CheckGameOver(const double& score)
{
	if(score == -INFINITY)
	{
		board.Reset();
		deaths++;
		avgBlocksPerGame = totalBlocks / double(deaths);
	}
	else
	{
		totalScore += score;
	}
}

Game::Game()
{
    if(MAX_WIDTH < BLOCKS_W)
    {
        Fatal("MAX_WIDTH < BLOCKS_W");
    }

	//init channel ptr
	findBestBoard_Rec_Channels = make_unique<FindBestBoard_Rec_Channels>(NUM_WORKERS);

    //init queue
    random_device device;
    rng = make_unique<mt19937>(device());
    dist = make_unique<std::uniform_int_distribution<std::mt19937::result_type>>(0,tetriminos.size() - 1);

    for(int i = 0; i < LOOK_AHEAD; i++)
    {
        tetriminoQueue.push_back((*dist)(*rng));
    }

	deaths = 0;
	totalBlocks = 0;
	avgBlocksPerGame = 0;
}

void Game::UpdateBoard(Board&& board)
{
    this->board = move(board);
}

void Game::UpdateQueue()
{
    tetriminoQueue.push_back((*dist)(*rng));
    tetriminoQueue.pop_front();
}

void Game::PrintFPS() const
{
	static int counter = 0;
	static auto last = std::chrono::high_resolution_clock::now();

	counter++;
	auto now = std::chrono::high_resolution_clock::now();
	auto dur = now - last;
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

	if(ms >= 1000)
	{
		cout << counter / (ms/1000.0) << " blocks/s" << endl;
		last = now;
		counter = 0;
		PrintStatistics();
		board.Print();
	}
}

void Game::PrintStatistics() const
{
	cout << "Avg Blocks Per Game: " << avgBlocksPerGame << "  Deaths: " << deaths << "  Avg Score: " << totalScore / totalBlocks << endl;
}

Board Game::FindBestBoard_SingleThread() const
{
	Board best;
	const Tetrimino& tetrimino = tetriminos[tetriminoQueue[0]];

	Helpers::ForEachTrPos(tetrimino, [this, &best](const TetriminoRotation& tr){
		Board localBoard(board);
		localBoard.DropAndUpdateScore(tr, [this, &localBoard, &best](const int& destroyedLines, const int& dropHeight, const TetriminoRotation& tr){
			localBoard.ResursiveScoreCalculator(destroyedLines, dropHeight, tr, best, tetriminoQueue);
		});
	});

	return best;
}

Board Game::FindBestBoard_MultiThread() const
{
	vector<future<Board>> futures;

	auto cmp = [](auto a, auto b){return a.first < b.first;};
	priority_queue<pair<double, Board>, vector<pair<double, Board>>, decltype(cmp)> scoresAndBoards(cmp);
	const Tetrimino& tetrimino = tetriminos[tetriminoQueue[0]];

	Helpers::ForEachTrPos(tetrimino, [this, &futures](TetriminoRotation& tr){
		Board localBoard(board);
		localBoard.DropAndUpdateScore(tr, [this, &localBoard, &futures](const int& destroyedLines, const int& dropHeight, const TetriminoRotation& tr){
			futures.emplace_back(async([localBoard, tetriminoQueue = tetriminoQueue, destroyedLines, dropHeight, tr](){
				Board best;
				localBoard.ResursiveScoreCalculator(destroyedLines, dropHeight, tr, best, tetriminoQueue);
				return best;
			}));
		});
	});

	Board best;
	for(auto& x: futures)
	{
		auto board = x.get();
		if(board.score > best.score)
			best = move(board);
	}
	return best;
}

void Game::Log(const Board& b)
{
	ofstream boardLogFile;
	string fileName = BOARDS_LOG_DIR + to_string(b.score);
	boardLogFile.open(fileName);
	if(!boardLogFile.is_open()){
		Log("Can't open file: " + fileName);
		throw;
	}
	boardLogFile << b.Serialize();
	boardLogFile.close();
}

void Game::Log(const Context& context)
{
	Board board(context.first);
	Tetrimino tetrimino(context.second);

	ofstream boardLogFile;
	string fileName = CONTEXTS_LOG_DIR + tetrimino.name + to_string(board.score);
	boardLogFile.open(fileName);
	if(!boardLogFile.is_open()){
		Log("Can't open file: " + fileName);
		throw;
	}
	boardLogFile << tetrimino.name << " " << board.Serialize();
	boardLogFile.close();
}

void Game::Log(const string& s)
{
	cout << s << endl;
}

void Game::Fatal(const string& err)
{
	Game::Log(err);
	throw;
}

const vector<Tetrimino> Game::tetriminos = Game::LoadTetriminos();

//Load tetriminos from files
vector<Tetrimino> Game::LoadTetriminos()
{
	vector<pair<string, int>> tetriminoData = {{"O", 1}, {"L", 4}, {"RL", 4}, {"N", 2}, {"RN", 2}, {"I", 2}, {"T", 4}};
	vector<Tetrimino> tetriminos;
	for(auto& data : tetriminoData) tetriminos.push_back(Tetrimino(data.first, data.second));
	return tetriminos;
}

Context Game::LoadContextFromFile(const string& fileName, const vector<Tetrimino>& tetriminos)
{
	ifstream contextFile;
	contextFile.open(fileName);
	if(!contextFile.is_open()){
		Game::Fatal("Cannot open file: " + fileName);
	}
	
	Board board;
	string tetriminoName;
	uint width;
	uint height;

	contextFile >> tetriminoName >> width >> height;

	Tetrimino tetrimino = *find_if(tetriminos.begin(), tetriminos.end(), [tetriminoName](auto t) {return t.name == tetriminoName;});

	if(width != BLOCKS_W || height != BLOCKS_H){
		Game::Fatal("Width or Height mismatch in loadcontextfromfile");
	}

	string line;
	int lineNumber = 0;
	getline(contextFile, line);
	while(getline(contextFile, line) && lineNumber < BLOCKS_H)
	{
		WidthInt currentWidthInt = 0;

		for(uint i = 0; i < line.size(); i++)
		{
			if(line.at(i) == '1')
			{
				currentWidthInt += ((WidthInt)1 << (MAX_WIDTH - 1 - i));
			}
		}
		board.boardArr[BLOCKS_H - 1 - lineNumber++] = currentWidthInt;
	}

	return Context(board, tetrimino);
}

FindBestBoard_Rec_Channels::FindBestBoard_Rec_Channels(const uint& numWorkers)
{
	this->numWorkers = numWorkers;

	//start all the coroutines
	for(uint i = 0; i < numWorkers; i++)
	{
		workers.push_back(async([this](){
			for(;;)
			{
				auto args = argsChan.value_pop();
				Board& localBoard = get<0>(args);
				TetriminoRotation& tr = get<1>(args);
				deque<int>& tetriminoQueue = get<2>(args);

				Board best;
				localBoard.DropAndUpdateScore(tr, [&tetriminoQueue, &best, &localBoard](const int& destroyedLines, const int& dropHeight, const TetriminoRotation& tr){
					localBoard.ResursiveScoreCalculator(destroyedLines, dropHeight, tr, best, tetriminoQueue);
				});
				resultChan.push(best);
			}
		}));
	}
}

Board FindBestBoard_Rec_Channels::operator()(const Board& board, const deque<int>& tetriminoQueue)
{
	uint workloadSize = 0;

	Helpers::ForEachTrPos(Game::tetriminos[tetriminoQueue[0]], [this, &board, &tetriminoQueue, &workloadSize](const TetriminoRotation& tr){
		Board localBoard(board);
		workloadSize++;
		argsChan.push(make_tuple(localBoard, tr, tetriminoQueue));
	});

	uint numResults = 0;

	Board best;
	for(auto& result: resultChan)
	{
		if(result.score > best.score)
			best = move(result);

		if(++numResults >= workloadSize)
			break;
	}

	return best;
}