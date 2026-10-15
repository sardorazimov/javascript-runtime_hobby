/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Declares local module resolution, execution, and caching behavior.
 */
#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <v8.h>

namespace mininode {

class Runtime;

class ModuleLoader {
public:
   explicit ModuleLoader(Runtime& runtime);
   ~ModuleLoader();

   void Install(v8::Local<v8::Context> context);
   bool RunMain(const std::filesystem::path& file);

private:
   static void RequireCallback(const v8::FunctionCallbackInfo<v8::Value>& info);
   v8::MaybeLocal<v8::Value> Require(v8::Local<v8::Context> context,
                             const std::string& request);
   v8::MaybeLocal<v8::Value> LoadFile(v8::Local<v8::Context> context,
                              const std::filesystem::path& file);
   std::filesystem::path Resolve(const std::string& request) const;

   Runtime& runtime_;
   std::unordered_map<std::string, v8::Global<v8::Object>> cache_;
   std::vector<std::filesystem::path> loading_stack_;
};

}