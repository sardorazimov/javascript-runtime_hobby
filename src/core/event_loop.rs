// Copyright (c) 2026 Sardor Azimov
// SPDX-License-Identifier: MIT
//! Runs Rust-owned timers and dispatches their JavaScript callbacks on the V8 thread.

use std::ffi::c_void;
use std::time::{Duration, Instant};

const CALLBACKS_NAME: &str = "__mini_timer_callbacks";

struct Timer {
    id: i32,
    deadline: Instant,
}

#[derive(Default)]
pub struct EventLoop {
    next_id: i32,
    timers: Vec<Timer>,
    callbacks: Option<v8::Global<v8::Array>>,
}

impl EventLoop {
    pub fn initialize<'s>(
        &mut self,
        scope: &mut v8::PinScope<'s, '_, v8::Context>,
    ) -> v8::Local<'s, v8::Array> {
        self.clear();
        let callbacks = v8::Array::new(scope, 0);
        self.callbacks = Some(v8::Global::new(scope, callbacks));
        callbacks
    }

    pub fn install(
        scope: &mut v8::PinScope,
        context: v8::Local<v8::Context>,
        callbacks: v8::Local<v8::Array>,
        event_loop_pointer: *mut EventLoop,
    ) {
        let global = context.global(scope);
        if let Some(name) = v8::String::new(scope, CALLBACKS_NAME) {
            global.set(scope, name.into(), callbacks.into());
        }

        let set_data = v8::External::new(scope, event_loop_pointer.cast::<c_void>());
        let set_template = v8::FunctionTemplate::builder(set_timeout_callback)
            .data(set_data.into())
            .build(scope);
        if let (Some(name), Some(function)) = (
            v8::String::new(scope, "setTimeout"),
            set_template.get_function(scope),
        ) {
            global.set(scope, name.into(), function.into());
        }

        let clear_data = v8::External::new(scope, event_loop_pointer.cast::<c_void>());
        let clear_template = v8::FunctionTemplate::builder(clear_timeout_callback)
            .data(clear_data.into())
            .build(scope);
        if let (Some(name), Some(function)) = (
            v8::String::new(scope, "clearTimeout"),
            clear_template.get_function(scope),
        ) {
            global.set(scope, name.into(), function.into());
        }
    }

    pub fn run<'s>(
        &mut self,
        scope: &mut v8::PinScope<'s, '_, ()>,
        context: v8::Local<'s, v8::Context>,
    ) {
        while !self.timers.is_empty() {
            self.timers.sort_by_key(|timer| timer.deadline);
            let delay = self.timers[0]
                .deadline
                .saturating_duration_since(Instant::now());
            if !delay.is_zero() {
                std::thread::sleep(delay);
            }

            let timer = self.timers.remove(0);
            let Some(callbacks) = self.callback_registry(scope) else {
                return;
            };
            let context_scope = &mut v8::ContextScope::new(scope, context);
            let Some(callback_value) = callbacks.get_index(context_scope, timer.id as u32) else {
                continue;
            };
            let Ok(callback) = v8::Local::<v8::Function>::try_from(callback_value) else {
                continue;
            };

            v8::tc_scope!(let try_catch, context_scope);
            let receiver = context.global(try_catch);
            if callback.call(try_catch, receiver.into(), &[]).is_none() {
                let message = try_catch
                    .exception()
                    .and_then(|exception| exception.to_string(try_catch))
                    .map(|message| message.to_rust_string_lossy(try_catch))
                    .unwrap_or_else(|| "unknown JavaScript exception".to_string());
                eprintln!("MiniRust timer error: {message}");
            }
            callbacks.set_index(try_catch, timer.id as u32, v8::undefined(try_catch).into());
        }
    }

    fn schedule(&mut self, delay_ms: u64) -> i32 {
        self.next_id += 1;
        self.timers.push(Timer {
            id: self.next_id,
            deadline: Instant::now() + Duration::from_millis(delay_ms),
        });
        self.next_id
    }

    fn cancel(&mut self, id: i32) {
        self.timers.retain(|timer| timer.id != id);
    }

    fn callback_registry<'s>(
        &self,
        scope: &v8::PinScope<'s, '_, ()>,
    ) -> Option<v8::Local<'s, v8::Array>> {
        self.callbacks
            .as_ref()
            .map(|callbacks| v8::Local::new(scope, callbacks))
    }

    pub fn clear(&mut self) {
        self.timers.clear();
        self.callbacks.take();
    }
}

fn event_loop_pointer(arguments: &v8::FunctionCallbackArguments) -> Option<*mut EventLoop> {
    let data = v8::Local::<v8::External>::try_from(arguments.data()).ok()?;
    let pointer = data.value().cast::<EventLoop>();
    (!pointer.is_null()).then_some(pointer)
}

fn callbacks_array<'s>(
    scope: &v8::PinScope<'s, '_, ()>,
    arguments: &v8::FunctionCallbackArguments,
) -> Option<v8::Local<'s, v8::Array>> {
    let pointer = event_loop_pointer(arguments)?;
    unsafe { (*pointer).callback_registry(scope) }
}

fn set_timeout_callback(
    scope: &mut v8::PinScope,
    arguments: v8::FunctionCallbackArguments,
    mut return_value: v8::ReturnValue,
) {
    if arguments.length() == 0 {
        crate::binding::v8_bridge::throw_error(scope, "setTimeout expects a callback");
        return;
    }
    if v8::Local::<v8::Function>::try_from(arguments.get(0)).is_err() {
        crate::binding::v8_bridge::throw_error(scope, "setTimeout expects a function");
        return;
    }
    let delay_ms = if arguments.length() > 1 {
        arguments
            .get(1)
            .integer_value(scope)
            .unwrap_or_default()
            .max(0) as u64
    } else {
        0
    };
    let Some(pointer) = event_loop_pointer(&arguments) else {
        crate::binding::v8_bridge::throw_error(scope, "event loop is unavailable");
        return;
    };
    let Some(callbacks) = callbacks_array(scope, &arguments) else {
        crate::binding::v8_bridge::throw_error(scope, "timer callback store is unavailable");
        return;
    };
    let id = unsafe { (*pointer).schedule(delay_ms) };
    if callbacks.set_index(scope, id as u32, arguments.get(0)) != Some(true) {
        unsafe {
            (*pointer).cancel(id);
        }
        crate::binding::v8_bridge::throw_error(scope, "could not register timer callback");
        return;
    }
    return_value.set(v8::Integer::new(scope, id).into());
}

fn clear_timeout_callback(
    scope: &mut v8::PinScope,
    arguments: v8::FunctionCallbackArguments,
    _return_value: v8::ReturnValue,
) {
    if arguments.length() == 0 {
        return;
    }
    let id = arguments.get(0).integer_value(scope).unwrap_or_default() as i32;
    if let Some(pointer) = event_loop_pointer(&arguments) {
        unsafe {
            (*pointer).cancel(id);
        }
        if let Some(callbacks) = callbacks_array(scope, &arguments) {
            callbacks.set_index(scope, id as u32, v8::undefined(scope).into());
        }
    }
}
