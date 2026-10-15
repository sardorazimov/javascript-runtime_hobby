// Copyright (c) 2026 Sardor Azimov
// SPDX-License-Identifier: MIT
//! Owns V8 startup, native module installation, script execution, and shutdown.

use std::path::{Path, PathBuf};

use crate::core::env::RuntimeEnvironment;
use crate::core::event_loop::EventLoop;
use crate::core::module_loader::ModuleLoader;
use crate::modules::buffer::BufferModule;
use crate::modules::console::ConsoleModule;
use crate::modules::fs::FsModule;

pub struct Runtime {
    environment: RuntimeEnvironment,
    event_loop: Box<EventLoop>,
    platform: Option<v8::SharedRef<v8::Platform>>,
    isolate: Option<v8::OwnedIsolate>,
    context: Option<v8::Global<v8::Context>>,
}

impl Runtime {
    pub fn new(working_directory: PathBuf) -> Result<Self, String> {
        let platform = v8::new_default_platform(0, false).make_shared();
        v8::V8::initialize_platform(platform.clone());
        v8::V8::initialize();

        let isolate = v8::Isolate::new(v8::CreateParams::default());
        Ok(Self {
            environment: RuntimeEnvironment::new(working_directory),
            event_loop: Box::default(),
            platform: Some(platform),
            isolate: Some(isolate),
            context: None,
        })
    }

    pub fn run_file(&mut self, script_path: &Path) -> Result<(), String> {
        let resolved_path = self.environment.resolve_script(script_path);
        let source = std::fs::read_to_string(&resolved_path)
            .map_err(|error| format!("cannot read {}: {error}", resolved_path.display()))?;

        let event_loop_pointer = &mut *self.event_loop as *mut EventLoop;
        let isolate = self
            .isolate
            .as_mut()
            .ok_or_else(|| "V8 isolate has already been disposed".to_string())?;
        v8::scope!(let scope, isolate);

        let context = v8::Context::new(scope, Default::default());
        self.context = Some(v8::Global::new(scope, context));
        {
            let scope = &mut v8::ContextScope::new(scope, context);
            ConsoleModule::install(scope, context);
            FsModule::install(scope, context);
            BufferModule::install(scope, context);
            ModuleLoader::install(scope, context);
            let callbacks = self.event_loop.initialize(scope);
            EventLoop::install(scope, context, callbacks, event_loop_pointer);

            let source = v8::String::new(scope, &source)
                .ok_or_else(|| "could not create a V8 source string".to_string())?;
            let script = v8::Script::compile(scope, source, None).ok_or_else(|| {
                format!("JavaScript compilation failed: {}", resolved_path.display())
            })?;
            script.run(scope).ok_or_else(|| {
                format!("JavaScript execution failed: {}", resolved_path.display())
            })?;
        }

        let context = self
            .context
            .as_ref()
            .map(|context| v8::Local::new(scope, context))
            .ok_or_else(|| "V8 context is unavailable".to_string())?;
        self.event_loop.run(scope, context);
        Ok(())
    }
}

impl Drop for Runtime {
    fn drop(&mut self) {
        self.event_loop.clear();
        self.context.take();
        self.isolate.take();
        unsafe {
            v8::V8::dispose();
        }
        v8::V8::dispose_platform();
        self.platform.take();
    }
}
