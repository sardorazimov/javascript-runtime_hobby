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
   static v8::MaybeLocal<v8::Module> ResolveEsModule(
      v8::Local<v8::Context> context, v8::Local<v8::String> specifier,
      v8::Local<v8::FixedArray> import_attributes,
      v8::Local<v8::Module> referrer);
   v8::MaybeLocal<v8::Value> Require(v8::Local<v8::Context> context,
                             const std::string& request);
   v8::MaybeLocal<v8::Value> LoadFile(v8::Local<v8::Context> context,
                              const std::filesystem::path& file);
   v8::MaybeLocal<v8::Module> LoadEsModule(
      v8::Local<v8::Context> context, const std::filesystem::path& file);
   bool RunEsModule(v8::Local<v8::Context> context,
                    const std::filesystem::path& file);
   std::filesystem::path Resolve(const std::string& request) const;
   std::filesystem::path ResolveEsModulePath(
      const std::string& request, const std::filesystem::path& referrer) const;

   Runtime& runtime_;
   std::unordered_map<std::string, v8::Global<v8::Object>> cache_;
   std::unordered_map<std::string, v8::Global<v8::Module>> es_module_cache_;
   std::unordered_map<int, std::filesystem::path> es_module_paths_;
   std::vector<std::filesystem::path> loading_stack_;
};

}