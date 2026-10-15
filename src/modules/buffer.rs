// Copyright (c) 2026 Sardor Azimov
// SPDX-License-Identifier: MIT
//! Implements a small V8-managed `Buffer` backed by `Uint8Array` values.

use crate::binding::v8_bridge;

const MAX_BUFFER_LENGTH: usize = 64 * 1024 * 1024;
const BUFFER_MARKER: &str = "__mini_rust_buffer";

pub struct BufferModule;

impl BufferModule {
    pub fn install(scope: &mut v8::PinScope, context: v8::Local<v8::Context>) {
        let buffer = Self::create_object(scope);
        let global = context.global(scope);
        if let Some(name) = v8::String::new(scope, "Buffer") {
            global.set(scope, name.into(), buffer.into());
        }
    }

    pub fn create_object<'s>(
        scope: &mut v8::PinScope<'s, '_, v8::Context>
    ) -> v8::Local<'s, v8::Object> {
        let buffer = v8::Object::new(scope);

        if let Some(name) = v8::String::new(scope, "alloc") {
            let function = v8::FunctionTemplate::new(scope, alloc);
            if let Some(function) = function.get_function(scope) {
                buffer.set(scope, name.into(), function.into());
            }
        }
        if let Some(name) = v8::String::new(scope, "from") {
            let function = v8::FunctionTemplate::new(scope, from);
            if let Some(function) = function.get_function(scope) {
                buffer.set(scope, name.into(), function.into());
            }
        }
        if let Some(name) = v8::String::new(scope, "isBuffer") {
            let function = v8::FunctionTemplate::new(scope, is_buffer);
            if let Some(function) = function.get_function(scope) {
                buffer.set(scope, name.into(), function.into());
            }
        }
        buffer
    }
}

fn make_buffer<'s>(
    scope: &mut v8::PinScope<'s, '_, v8::Context>,
    bytes: Vec<u8>
) -> Option<v8::Local<'s, v8::Uint8Array>> {
    if bytes.len() > MAX_BUFFER_LENGTH {
        return None;
    }
    let length = bytes.len();
    let backing_store = v8::ArrayBuffer
        ::new_backing_store_from_bytes(bytes.into_boxed_slice())
        .make_shared();
    let array_buffer = v8::ArrayBuffer::with_backing_store(scope, &backing_store);
    let buffer = v8::Uint8Array::new(scope, array_buffer, 0, length)?;

    if let Some(marker) = v8::String::new(scope, BUFFER_MARKER) {
        buffer.set(scope, marker.into(), v8::Boolean::new(scope, true).into());
    }
    if let Some(name) = v8::String::new(scope, "toString") {
        let function = v8::FunctionTemplate::new(scope, to_string);
        if let Some(function) = function.get_function(scope) {
            buffer.set(scope, name.into(), function.into());
        }
    }
    Some(buffer)
}

fn alloc(
    scope: &mut v8::PinScope,
    arguments: v8::FunctionCallbackArguments,
    mut return_value: v8::ReturnValue
) {
    if arguments.length() == 0 {
        v8_bridge::throw_error(scope, "Buffer.alloc expects a byte length");
        return;
    }
    let length = arguments.get(0).integer_value(scope).unwrap_or(-1);
    if length < 0 || (length as usize) > MAX_BUFFER_LENGTH {
        v8_bridge::throw_error(scope, "Buffer length must be between 0 and 64 MiB");
        return;
    }
    let context = scope.get_current_context();
    let context_scope = &mut v8::ContextScope::new(scope, context);
    if let Some(buffer) = make_buffer(context_scope, vec![0; length as usize]) {
        return_value.set(buffer.into());
    }
}

fn from(
    scope: &mut v8::PinScope,
    arguments: v8::FunctionCallbackArguments,
    mut return_value: v8::ReturnValue
) {
    if arguments.length() == 0 {
        v8_bridge::throw_error(scope, "Buffer.from expects a string or Uint8Array");
        return;
    }

    let input = arguments.get(0);
    let bytes = if input.is_string() {
        let Some(text) = v8_bridge::to_rust_string(scope, input) else {
            v8_bridge::throw_error(scope, "could not convert Buffer input to text");
            return;
        };
        text.into_bytes()
    } else if let Ok(array) = v8::Local::<v8::Uint8Array>::try_from(input) {
        let mut storage = vec![0; array.byte_length()];
        array.get_contents(&mut storage).to_vec()
    } else {
        v8_bridge::throw_error(scope, "Buffer.from supports strings and Uint8Array values");
        return;
    };

    if bytes.len() > MAX_BUFFER_LENGTH {
        v8_bridge::throw_error(scope, "Buffer length exceeds 64 MiB");
        return;
    }
    let context = scope.get_current_context();
    let context_scope = &mut v8::ContextScope::new(scope, context);
    if let Some(buffer) = make_buffer(context_scope, bytes) {
        return_value.set(buffer.into());
    }
}

fn is_buffer(
    scope: &mut v8::PinScope,
    arguments: v8::FunctionCallbackArguments,
    mut return_value: v8::ReturnValue
) {
    if arguments.length() == 0 || !arguments.get(0).is_object() {
        return_value.set(v8::Boolean::new(scope, false).into());
        return;
    }
    let object = arguments.get(0).to_object(scope).unwrap();
    let is_buffer = v8::String
        ::new(scope, BUFFER_MARKER)
        .and_then(|name| object.get(scope, name.into()))
        .is_some_and(|marker| marker.is_true());
    return_value.set(v8::Boolean::new(scope, is_buffer).into());
}

fn to_string(
    scope: &mut v8::PinScope,
    arguments: v8::FunctionCallbackArguments,
    mut return_value: v8::ReturnValue
) {
    let Ok(array) = v8::Local::<v8::Uint8Array>::try_from(arguments.this()) else {
        v8_bridge::throw_error(scope, "Buffer.toString called on an incompatible value");
        return;
    };
    let mut storage = vec![0; array.byte_length()];
    let text = String::from_utf8_lossy(array.get_contents(&mut storage)).into_owned();
    if let Some(text) = v8::String::new(scope, &text) {
        return_value.set(text.into());
    }
}
