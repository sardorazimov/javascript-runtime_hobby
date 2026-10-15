// Copyright (c) 2026 Sardor Azimov
// SPDX-License-Identifier: MIT
//! Implements the native `console.log()` binding.

pub struct ConsoleModule;

impl ConsoleModule {
    pub fn install(scope: &mut v8::PinScope, context: v8::Local<v8::Context>) {
        let console = v8::Object::new(scope);
        let log_template = v8::FunctionTemplate::new(scope, log_callback);
        let Some(log) = log_template.get_function(scope) else {
            return;
        };
        let Some(log_name) = v8::String::new(scope, "log") else {
            return;
        };
        console.set(scope, log_name.into(), log.into());

        let Some(console_name) = v8::String::new(scope, "console") else {
            return;
        };
        context
            .global(scope)
            .set(scope, console_name.into(), console.into());
    }
}

fn log_callback(
    scope: &mut v8::PinScope,
    arguments: v8::FunctionCallbackArguments,
    _return_value: v8::ReturnValue,
) {
    let values = (0..arguments.length())
        .filter_map(|index| {
            arguments
                .get(index)
                .to_string(scope)
                .map(|value| value.to_rust_string_lossy(scope))
        })
        .collect::<Vec<_>>();
    println!("{}", values.join(" "));
}
