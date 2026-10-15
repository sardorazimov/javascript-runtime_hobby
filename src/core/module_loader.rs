// Copyright (c) 2026 Sardor Azimov
// SPDX-License-Identifier: MIT
//! Dispatches the first built-in modules exposed through `require()`.

use crate::binding::v8_bridge;
use crate::modules::buffer::BufferModule;
use crate::modules::fs::FsModule;

pub struct ModuleLoader;

impl ModuleLoader {
    pub fn install(scope: &mut v8::PinScope, context: v8::Local<v8::Context>) {
        let global = context.global(scope);
        let Some(name) = v8::String::new(scope, "require") else {
            return;
        };
        let function = v8::FunctionTemplate::new(scope, require_callback);
        let Some(function) = function.get_function(scope) else {
            return;
        };
        global.set(scope, name.into(), function.into());
    }
}

fn require_callback(
    scope: &mut v8::PinScope,
    arguments: v8::FunctionCallbackArguments,
    mut return_value: v8::ReturnValue
) {
    if arguments.length() == 0 {
        v8_bridge::throw_error(scope, "require expects a built-in module name");
        return;
    }

    let Some(name) = v8_bridge::to_rust_string(scope, arguments.get(0)) else {
        v8_bridge::throw_error(scope, "module name must be a string");
        return;
    };

    match name.as_str() {
        "fs" => return_value.set(FsModule::create_object(scope).into()),
        "buffer" => {
            let context = scope.get_current_context();
            let context_scope = &mut v8::ContextScope::new(scope, context);
            return_value.set(BufferModule::create_object(context_scope).into());
        }
        "console" => {
            let context = scope.get_current_context();
            if let Some(console_name) = v8::String::new(scope, "console") {
                if let Some(console) = context.global(scope).get(scope, console_name.into()) {
                    return_value.set(console);
                }
            }
        }
        _ => v8_bridge::throw_error(scope, &format!("unknown built-in module: {name}")),
    }
}
