/*-------------15-01-2026-------------*/
#pragma once
#include <iomanip>
#include <semaphore>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"


std::binary_semaphore sem3(1);

template<typename... T>
void Print(T... x) {
    (std::cout << ... << x);
    std::cout << std::endl; 
}