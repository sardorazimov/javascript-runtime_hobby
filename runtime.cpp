#include "runtime.h"

#include <iostream>
#include <stdexcept>
#include <utility>

#include "buffer.h"
#include "console_module.h"
#include "event_loop.h"
#include "fs_module.h"
#include "module_loader.h"
#include "net_module.h"
#include "promise_tracker.h"

namespace mininode {

Runtime::Runtime(std::filesystem::path working_directory)
   : environment_{std::filesystem::absolute(std::move(working_directory)), {}} {
   platform_ = v8::platform::NewDefaultPlatform();
   v8::V8::InitializePlatform(platform_.get());
   if (!v8::V8::Initialize()) {
      v8::V8::DisposePlatform();
      throw std::runtime_error("V8 initialization failed");
   }

   allocator_.reset(v8::ArrayBuffer::Allocator::NewDefaultAllocator());
   v8::Isolate::CreateParams create_params;
   create_params.array_buffer_allocator = allocator_.get();
   isolate_ = v8::Isolate::New(create_params);
   if (isolate_ == nullptr) {
      v8::V8::Dispose();
      v8::V8::DisposePlatform();
      throw std::runtime_error("Could not create a V8 isolate");
   }

   isolate_->SetData(0, this);
   isolate_->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);

   {
      v8::Isolate::Scope isolate_scope(isolate_);
      v8::HandleScope handle_scope(isolate_);
      auto local_context = v8::Context::New(isolate_);
      context_.Reset(isolate_, local_context);
      v8::Context::Scope context_scope(local_context);

      ConsoleModule::Install(isolate_, local_context);
      event_loop_ = std::make_unique<EventLoop>(isolate_);
      fs_async_context_.isolate = isolate_;
      fs_async_context_.context.Reset(isolate_, local_context);
      fs_async_context_.task_pool = &async_tasks_;
      fs_async_context_.event_loop = event_loop_.get();
      FsModule::Install(isolate_, local_context, &fs_async_context_);
      BufferModule::Install(isolate_, local_context);
      NetModule::Install(isolate_, local_context);
      event_loop_->Install(local_context);
      module_loader_ = std::make_unique<ModuleLoader>(*this);
      module_loader_->Install(local_context);
      PromiseTracker::Install(isolate_);
   }
}

Runtime::~Runtime() {
   if (isolate_ != nullptr) {
      {
         v8::Isolate::Scope isolate_scope(isolate_);
         v8::HandleScope handle_scope(isolate_);
         event_loop_.reset();
         module_loader_.reset();
         fs_async_context_.context.Reset();
         context_.Reset();
      }
      isolate_->Dispose();
      isolate_ = nullptr;
   }
   v8::V8::Dispose();
   v8::V8::DisposePlatform();
}

bool Runtime::RunFile(const std::filesystem::path& file) {
   std::error_code error;
   const auto absolute_file = std::filesystem::weakly_canonical(
      file.is_absolute() ? file : environment_.working_directory / file, error);
   if (error || !std::filesystem::is_regular_file(absolute_file)) {
      std::cerr << "MiniNode: script not found: " << file << '\n';
      return false;
   }

   environment_.main_script = absolute_file;
   environment_.working_directory = absolute_file.parent_path();

   bool succeeded = false;
   {
      v8::Isolate::Scope isolate_scope(isolate_);
      v8::HandleScope handle_scope(isolate_);
      auto local_context = context();
      v8::Context::Scope context_scope(local_context);
      v8::TryCatch try_catch(isolate_);
      succeeded = module_loader_->RunMain(absolute_file);
      if (!succeeded && !try_catch.Exception().IsEmpty()) {
         v8::String::Utf8Value message(isolate_, try_catch.Exception());
         std::cerr << "MiniNode: " << (*message == nullptr ? "JavaScript error" : *message)
                 << '\n';
      }
      isolate_->PerformMicrotaskCheckpoint();
      if (succeeded) {
         event_loop_->Run(local_context);
      }
   }
   return succeeded;
}

v8::Isolate* Runtime::isolate() const noexcept {
   return isolate_;
}

v8::Local<v8::Context> Runtime::context() const {
   return context_.Get(isolate_);
}

RuntimeEnvironment& Runtime::environment() noexcept {
   return environment_;
}

FsModule::AsyncContext& Runtime::fs_async_context() noexcept {
   return fs_async_context_;
}

}
/* runtime.h: V8 Isolate, Context ve global state yönetimini 
   tek bir çatı altında toplayan ana sınıfın tanımıdır. */