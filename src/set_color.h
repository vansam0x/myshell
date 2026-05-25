#pragma once
#include <iomanip>
#include <iostream>
#include <semaphore>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

inline std::binary_semaphore sem3(1);

template<typename... T>
void Print(T... x) {
    sem3.acquire();
    (cout << ... << x) << RESET;
    sem3.release();
}