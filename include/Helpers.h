#pragma once

#include <functional>
#include <chrono>
#include <iostream>
#include <string>

#include "Tetrimino.h"

namespace Helpers
{
    template<typename T, typename... Args>
    std::function<T(Args...)> Timer(const std::function<T(Args...)>& func, std::ostream& os, const std::string& funcName)
    {
        return [&func, &os, funcName](Args... args){
            auto begin = std::chrono::high_resolution_clock::now();
            T ret = func(args...);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto dur = end - begin;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
            
            os << "Function " << funcName << " took " << ms << " ms." << std::endl;

            return ret;
        };
    }

    template<typename... Args>
    std::function<void(Args...)> Timer(const std::function<void(Args...)>&& func, std::ostream& os, const std::string& funcName)
    {
        return [&func, &os, funcName](Args... args){
            auto begin = std::chrono::high_resolution_clock::now();
            func(args...);
            auto end = std::chrono::high_resolution_clock::now(); 
            
            auto dur = end - begin;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
            
            os << "Function " << funcName << " took " << ms << " ms." << std::endl;
        };
    }

    template <typename Func>
    void ForEachTrPos(const Tetrimino& tetrimino, const Func&& f)
    {
        for(TetriminoRotation tetriminoRotation : tetrimino.tetriminoRotations)
        {
            for(int i = 0; i < BLOCKS_W - tetriminoRotation.width + 1; i++)
            {
                f(tetriminoRotation);
                
                for(auto& piece : tetriminoRotation.piece)
                {
                    piece >>= 1;
                }
            }
        }
    }

    void Pause(std::string msg);
}