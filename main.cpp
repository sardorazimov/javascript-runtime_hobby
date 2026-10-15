/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Parses the script path, starts MiniNode, and runs the entry script.
 */
#include <filesystem>
#include <exception>
#include <iostream>

#include "runtime.h"

int main(int argc, char* argv[]) {
    const std::filesystem::path script = argc > 1 ? argv[1] : "app.js";
    try {
        mininode::Runtime runtime(std::filesystem::current_path());
        return runtime.RunFile(script) ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "MiniNode startup error: " << error.what() << '\n';
        return 1;
    }
}


/*. This code only compiles .cpp and .h files and has nothing
 to do with Rust; Rust is different—it will have a separate runtime for learning purposes.
 So, for now, there are two different languages ​​involved. */