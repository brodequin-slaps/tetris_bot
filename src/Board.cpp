#include <algorithm>
#include <iostream>
#include <boost/functional/hash.hpp>

#include "Board.h"
#include "Helpers.h"
#include "Game.h"

using namespace std;

Board::Board()
{
    Reset();
}

void Board::Reset()
{
	for_each(boardArr.begin(), boardArr.end(), [](auto& line) { line = 0;});
    score = -INFINITY;
}

int getRowTransitions(const Board& board)
{
	int transitions = 0;
	bool last_bit = 1;
	bool bit;

	for (int i = 0; i <= BLOCKS_H - 1; i++)
	{
		WidthInt row = board.boardArr[i];

		for (int j = 0; j < BLOCKS_W; j++)
		{
			bit = (row >> (MAX_WIDTH - BLOCKS_W + j)) & (WidthInt)1;

			if (bit != last_bit)
				++transitions;

			last_bit = bit;
		}
		if (bit == 0)
			++transitions;
		last_bit = 1;
	}
	return transitions;
}

int getColumnTransitions(const Board& board)
{
	int transitions = 0;
	bool last_bit = 1;
    int startingHeight = BLOCKS_H -1;

	for (int i = 0; i < BLOCKS_W; ++i) {
		for (int j = 0; j <= startingHeight; ++j) {
			WidthInt row = board.boardArr[j];
			bool bit = (row >> (MAX_WIDTH - BLOCKS_W + i)) & (WidthInt)1;

			if (bit != last_bit) {
				++transitions;
			}

			last_bit = bit;
		}

		last_bit = 1;
	}

	return transitions;
}

int getNumberOfHoles(const Board& board) {
	int holes = 0;
	WidthInt row_holes = 0;
    int startingHeight = BLOCKS_H - 1;
	WidthInt previous_row = board.boardArr[startingHeight];

	for (int i = startingHeight - 1; i >= 0; --i) {
		row_holes = ~board.boardArr[i] & (previous_row | row_holes);

		for (int j = 0; j < BLOCKS_W; ++j) {
			holes += ((row_holes >> (MAX_WIDTH - BLOCKS_W + j)) & (WidthInt)1);
		}

		previous_row = board.boardArr[i];
	}

	return holes;
}


int getWellSums(const Board& board) {
	int well_sums = 0;

	// Check for well cells in the "inner columns" of the board.
	// "Inner columns" are the columns that aren't touching the edge of the board.
    int startingHeight = BLOCKS_H - 1;

	for (int i = 1; i < BLOCKS_W - 1; ++i) {
		for (int j = startingHeight; j >= 0; --j) {
			if (((board.boardArr[j] >> (MAX_WIDTH - BLOCKS_W + i - 1)) & (WidthInt)7) == (WidthInt)5) {

				// Found well cell, count it + the number of empty cells below it.
				++well_sums;
				
				for (int k = j - 1; k >= 0; --k) {
					if (((board.boardArr[k] >> (MAX_WIDTH - BLOCKS_W + i)) & (WidthInt)1) == (WidthInt)0) {
						++well_sums;
					}
					else {
						break;
					}
				}
			}
		}
	}

	// Check for well cells in the leftmost column of the board.
	for (int j = startingHeight; j >= 0; --j) {
		if (((board.boardArr[j] >> (MAX_WIDTH - BLOCKS_W)) & (WidthInt)3) == (WidthInt)2) {

			// Found well cell, count it + the number of empty cells below it.
			++well_sums;

			for (int k = j - 1; k >= 0; --k) {
				if (((board.boardArr[k] >> (MAX_WIDTH - BLOCKS_W)) & (WidthInt)1) == (WidthInt)0) {
					++well_sums;
				}
				else {
					break;
				}
			}
		}
	}

	// Check for well cells in the rightmost column of the board.
	for (int j = startingHeight; j >= 0; --j) {
		if (((board.boardArr[j] >> (MAX_WIDTH-2)) & (WidthInt)3) == (WidthInt)1) {
			// Found well cell, count it + the number of empty cells below it.

			++well_sums;
			for (int k = j - 1; k >= 0; --k) {
				if (((board.boardArr[k] >> (MAX_WIDTH - 1)) & (WidthInt)1) == (WidthInt)0) {
					++well_sums;
				}
				else {
					break;
				}
			}
		}
	}

	return well_sums;
}

void Board::SetScore(const int& destroyedLines, const int& dropHeight, const int& tetriminoRotationHeight)
{
    score = CalculateScore(destroyedLines, dropHeight, tetriminoRotationHeight);
}

double Board::CalculateScore(const int& destroyedLines, const int& dropHeight, const int& tetriminoRotationHeight) const
{
    double adjustedDropHeight = dropHeight + ((tetriminoRotationHeight - 1)/2);
    return (double)adjustedDropHeight * -4.500158825082766 +
        (double)destroyedLines * 3.4181268101392694 +
        (double)getRowTransitions(*this) * -3.2178882868487753 +
        (double)getColumnTransitions(*this) * -9.348695305445199 +
        (double)getNumberOfHoles(*this) * -7.899265427351652 +
        (double)getWellSums(*this) * -3.3855972247263626;
}

int Board::DropTetriminoRotation(const TetriminoRotation& tr)
{
    int height = BLOCKS_H - 1;
    for(;;)
    {
        for(int i = tr.height - 1; i >= 0; i--)
        {
            WidthInt result = boardArr[height - i] | tr.piece[i];
            
            //if collision
            if(result != (boardArr[height - i] ^ tr.piece[i]))
            {
                height++;
                if(height + tr.height > BLOCKS_H)
                    return -1;
                
                for(int j = 0; j < tr.height; j++)
                {
                    boardArr[height - j] |= tr.piece[j];
                }
                return height;
            }
        }
        height--;
        if(height - tr.height < -1)
        {
            height++;
            //place on ground
            for(int j = 0; j < tr.height; j++)
            {
                boardArr[height - j] |= tr.piece[j];
            }
			return height;
        }
    }
}

//returns 1 if destroyed line, 0 if no line destroyed
int destroySingleLine(board_t& board, const int& height)
{
	if (board[height] == FULL_LINE)
	{
		for (int i = height; i < BLOCKS_H - 1; i++)
		{
			board[i] = board[i + 1];
		}
		board[BLOCKS_H - 1] = 0;
		return 1;
	}
	return 0;
}


int Board::DestroyLines(const int& dropHeight, const int& tetriminoRotationHeight)
{
    int d = 0;
	int toSend = dropHeight - tetriminoRotationHeight + 1;
	for (int i = toSend; i <= dropHeight; i++)
	{
		int destroyed = destroySingleLine(boardArr, toSend);
		d += destroyed;
		if (!destroyed)
			toSend++;
	}
	return d;
}

double Board::BestSubScore(const int& currentDepth, const int& maxDepth, const deque<int>& tetriminoQueue) const
{
	double best = -INFINITY;
    const Tetrimino& tetrimino = Game::tetriminos[tetriminoQueue[currentDepth]];

    Helpers::ForEachTrPos(tetrimino, [this, &best, currentDepth, maxDepth, tetriminoQueue](const TetriminoRotation& tr){
        Board localBoard(*this);

        localBoard.DropAndUpdateScore(tr, [&localBoard, &best, currentDepth, maxDepth, tetriminoQueue](const int& destroyedLines, const int& dropHeight, const TetriminoRotation& tr){
            localBoard.SubScoreCalculator(destroyedLines, dropHeight, tr, best, currentDepth, maxDepth, tetriminoQueue);
        });
    });

	return best;
}

void Board::ResursiveScoreCalculator(const int& destroyedLines, const int& dropHeight, const TetriminoRotation& tr, Board& best, const deque<int>& tetriminoQueue) const
{
    double score;
    if(LOOK_AHEAD == 1)
        score = CalculateScore(destroyedLines, dropHeight, tr.height);
    else
        score = BestSubScore(1, LOOK_AHEAD - 1, tetriminoQueue);

    if(score > best.score)
    {
        best = *this;
        best.score = score;
    }
}

void Board::SubScoreCalculator(const int& destroyedLines, const int& dropHeight, const TetriminoRotation& tr, double& best, const int& currentDepth, const int& maxDepth, const std::deque<int>& tetriminoQueue) const
{
    double score;

    if(currentDepth == maxDepth)
        score = CalculateScore(destroyedLines, dropHeight, tr.height);
    else
        score = BestSubScore(currentDepth + 1, maxDepth, tetriminoQueue);

    if(score > best)
        best = score;
}


void Board::Print(int spaces) const
{
    for (int i = BLOCKS_H - 1; i >= 0; i--)
    {
        for (int j = 0; j < BLOCKS_W; j++)
        {
            WidthInt a = boardArr[i];
            bool x = (a >> (MAX_WIDTH - 1 - j)) & 1;
            //cout << x << " ";
            if(x){
                cout << "x ";
            } else{
                cout << ". ";
            }
        }
        cout << endl;
    }

    for(int i = 0; i < spaces; i++)
    {
        cout << endl;
    }
}

//format: first line is: BLOCKS_W BLOCKS_H
//next is boardArr with 0s and 1s
string Board::Serialize() const
{
    string out = "";

    //first line
    out += to_string(BLOCKS_W) + " ";
    out += to_string(BLOCKS_H) + '\n';

    //board
    for (int i = BLOCKS_H - 1; i >= 0; i--)
    {
        for (int j = 0; j < BLOCKS_W; j++)
        {
            WidthInt a = boardArr[i];
            bool x = (a >> (MAX_WIDTH - 1 - j)) & 1;
            if(x){
                out += "1";
            } else{
                out += "0";
            }
        }
        out += '\n';
    }
    return out;
}

bool operator==(const Board& lhs, const Board& rhs)
{
    return lhs.boardArr == rhs.boardArr;
}

size_t hash_value(const Board& board)
{
    boost::hash<board_t> hasher;
    return hasher(board.boardArr);
}