/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Stores paths and other state shared by a MiniNode runtime instance.
 */
#pragma once

#include <filesystem>

namespace mininode {

struct RuntimeEnvironment {
   std::filesystem::path working_directory;
   std::filesystem::path main_script;
};

}