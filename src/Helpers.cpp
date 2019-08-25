#include "Helpers.h"

void Helpers::Pause(std::string msg)
{
    std::cout << msg << ". Press to continue.";
    std::cin.ignore();
}
