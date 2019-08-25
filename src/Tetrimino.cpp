#include <iostream>
#include <fstream>
#include <string>

#include "Tetrimino.h"
#include "Constants.h"
#include "Game.h"

using namespace std;

Tetrimino::Tetrimino(const string& name, int numRotations)
{
    this->name = name;
	//pour chaque fichier dans le dossier name
	//ajouter une concretePiece dans pieces
	string fileName;
	ifstream file;

	for (int i = 1; i <= numRotations; i++)
	{
		fileName = TETRIMINO_DIR + name + "/" + to_string(i) + ".txt";
		
		file.open(fileName.c_str());
        if(!file.is_open())
            Game::Fatal("Failed to open " + fileName);

        TetriminoRotation tetriminoRotation;
        string s;
        while(getline(file, s))
        {
            WidthInt line = 0;
            for(int j = 0; j < s.size(); j++)
            {
                if(s[j] == '1')
                    line += pow(2, MAX_WIDTH - 1 - j);
            }
            tetriminoRotation.piece.push_back(line);
            tetriminoRotation.width = s.size();
        }
    
        tetriminoRotation.height = tetriminoRotation.piece.size();
        tetriminoRotations.push_back(tetriminoRotation);
        file.close();
	}
}