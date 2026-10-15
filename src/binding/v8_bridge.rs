// Copyright (c) 2026 Sardor Azimov
// SPDX-License-Identifier: MIT
//! Converts common V8 values and raises JavaScript errors from Rust bindings.

pub fn to_rust_string(scope: &mut v8::PinScope, value: v8::Local<v8::Value>) -> Option<String> {
    value
        .to_string(scope)
        .map(|value| value.to_rust_string_lossy(scope))
}

pub fn throw_error(scope: &mut v8::PinScope, message: &str) {
    let Some(message) = v8::String::new(scope, message) else {
        return;
    };
    let error = v8::Exception::error(scope, message);
    scope.throw_exception(error);
}
