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