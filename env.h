#pragma once

#include <filesystem>

namespace mininode {

struct RuntimeEnvironment {
   std::filesystem::path working_directory;
   std::filesystem::path main_script;
};

}
/* env.h: Global değişkenleri, thread context'lerini ve 
   runtime genelinde paylaşılması gereken state'leri tutar. */