/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Declares the owner of V8 state and the MiniNode runtime lifecycle.
 */
#pragma once

#include <filesystem>
#include <memory>

#include <libplatform/libplatform.h>
#include <v8.h>

#include "async_task.h"
#include "env.h"
#include "fs_module.h"

namespace mininode {

class EventLoop;
class ModuleLoader;

class Runtime {
public:
   explicit Runtime(std::filesystem::path working_directory);
   ~Runtime();

   Runtime(const Runtime&) = delete;
   Runtime& operator=(const Runtime&) = delete;

   bool RunFile(const std::filesystem::path& file);
   v8::Isolate* isolate() const noexcept;
   v8::Local<v8::Context> context() const;
   RuntimeEnvironment& environment() noexcept;
   FsModule::AsyncContext& fs_async_context() noexcept;

private:
   RuntimeEnvironment environment_;
   AsyncTaskPool async_tasks_;
   FsModule::AsyncContext fs_async_context_;
   std::unique_ptr<v8::Platform> platform_;
   std::unique_ptr<v8::ArrayBuffer::Allocator> allocator_;
   v8::Isolate* isolate_ = nullptr;
   v8::Global<v8::Context> context_;
   std::unique_ptr<EventLoop> event_loop_;
   std::unique_ptr<ModuleLoader> module_loader_;
};

}