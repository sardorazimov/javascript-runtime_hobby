#include "module_loader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "binding.h"
#include "buffer.h"
#include "console_module.h"
#include "fs_module.h"
#include "net_module.h"
#include "runtime.h"

namespace mininode {

ModuleLoader::ModuleLoader(Runtime& runtime) : runtime_(runtime) {}

ModuleLoader::~ModuleLoader() {
   cache_.clear();
}

void ModuleLoader::Install(v8::Local<v8::Context> context) {
   auto* isolate = v8::Isolate::GetCurrent();
   auto data = v8::External::New(
      isolate, this, v8::kExternalPointerTypeTagDefault);
   auto require = v8::Function::New(context, RequireCallback, data).ToLocalChecked();
   context->Global()->Set(context, ToV8String(isolate, "require"), require).Check();
}

bool ModuleLoader::RunMain(const std::filesystem::path& file) {
   return !LoadFile(runtime_.context(), file).IsEmpty();
}

void ModuleLoader::RequireCallback(
   const v8::FunctionCallbackInfo<v8::Value>& info) {
   auto* isolate = info.GetIsolate();
   if (info.Length() == 0) {
      isolate->ThrowException(v8::Exception::TypeError(
         ToV8String(isolate, "require expects a module name")));
      return;
   }

   auto* loader = static_cast<ModuleLoader*>(
      info.Data().As<v8::External>()->Value(v8::kExternalPointerTypeTagDefault));
   try {
      const auto request = ToStdString(isolate, info[0]);
      v8::Local<v8::Value> result;
      if (loader->Require(isolate->GetCurrentContext(), request).ToLocal(&result)) {
         info.GetReturnValue().Set(result);
      }
   } catch (const std::exception& error) {
      isolate->ThrowException(v8::Exception::Error(
         ToV8String(isolate, error.what())));
   }
}

v8::MaybeLocal<v8::Value> ModuleLoader::Require(
   v8::Local<v8::Context> context, const std::string& request) {
   if (request == "fs") {
      auto* runtime = static_cast<Runtime*>(
         v8::Isolate::GetCurrent()->GetData(0));
      return FsModule::CreateObject(context, &runtime->fs_async_context());
   }
   if (request == "console") {
      return context->Global()->Get(
         context, ToV8String(v8::Isolate::GetCurrent(), "console"));
   }
   if (request == "buffer") {
      return BufferModule::CreateObject(context);
   }
   if (request == "net") {
      return NetModule::CreateObject(context);
   }
   return LoadFile(context, Resolve(request));
}

std::filesystem::path ModuleLoader::Resolve(const std::string& request) const {
   const std::filesystem::path requested(request);
   auto base = runtime_.environment().working_directory;
   if (!loading_stack_.empty()) {
      base = loading_stack_.back().parent_path();
   }

   auto candidate = requested.is_absolute() ? requested : base / requested;
   std::error_code error;
   if (std::filesystem::is_regular_file(candidate, error)) {
      return std::filesystem::weakly_canonical(candidate, error);
   }
   candidate += ".js";
   if (std::filesystem::is_regular_file(candidate, error)) {
      return std::filesystem::weakly_canonical(candidate, error);
   }

   throw std::runtime_error("Cannot find module '" + request + "'");
}

v8::MaybeLocal<v8::Value> ModuleLoader::LoadFile(
   v8::Local<v8::Context> context, const std::filesystem::path& file) {
   auto* isolate = v8::Isolate::GetCurrent();
   std::error_code error;
   const auto canonical_file = std::filesystem::weakly_canonical(file, error);
   if (error) {
      isolate->ThrowException(v8::Exception::Error(
         ToV8String(isolate, "Cannot resolve module: " + file.string())));
      return {};
   }

   const auto cache_key = canonical_file.string();
   if (const auto cached = cache_.find(cache_key); cached != cache_.end()) {
      return cached->second.Get(isolate)->Get(
         context, ToV8String(isolate, "exports"));
   }

   std::ifstream source_file(canonical_file, std::ios::binary);
   if (!source_file) {
      isolate->ThrowException(v8::Exception::Error(
         ToV8String(isolate, "Cannot read module: " + canonical_file.string())));
      return {};
   }
   std::ostringstream source_stream;
   source_stream << source_file.rdbuf();

   auto module = v8::Object::New(isolate);
   auto exports = v8::Object::New(isolate);
   module->Set(context, ToV8String(isolate, "exports"), exports).Check();
   cache_.emplace(cache_key, v8::Global<v8::Object>(isolate, module));

   const std::string wrapped_source =
      "(function(exports, require, module, __filename, __dirname) {\n" +
      source_stream.str() + "\n})";
   v8::Local<v8::Script> script;
   if (!v8::Script::Compile(context, ToV8String(isolate, wrapped_source))
          .ToLocal(&script)) {
      cache_.erase(cache_key);
      return {};
   }

   v8::Local<v8::Value> wrapper_value;
   if (!script->Run(context).ToLocal(&wrapper_value) || !wrapper_value->IsFunction()) {
      cache_.erase(cache_key);
      return {};
   }

   auto require_data = v8::External::New(
      isolate, this, v8::kExternalPointerTypeTagDefault);
   auto require = v8::Function::New(context, RequireCallback, require_data)
                  .ToLocalChecked();
   v8::Local<v8::Value> arguments[] = {
      exports,
      require,
      module,
      ToV8String(isolate, canonical_file.string()),
      ToV8String(isolate, canonical_file.parent_path().string())};

   loading_stack_.push_back(canonical_file);
   const bool invoked = !wrapper_value.As<v8::Function>()
                       ->Call(context, v8::Undefined(isolate), 5, arguments)
                       .IsEmpty();
   loading_stack_.pop_back();
   if (!invoked) {
      cache_.erase(cache_key);
      return {};
   }

   return module->Get(context, ToV8String(isolate, "exports"));
}

}