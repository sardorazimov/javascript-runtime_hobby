// Copyright (c) 2026 Sardor Azimov
// SPDX-License-Identifier: MIT
//! Implements synchronous text-file operations exposed through `require("fs")`.

use std::fs;
use std::path::Path;

use crate::binding::v8_bridge;

pub struct FsModule;

impl FsModule {
    pub fn install(scope: &mut v8::PinScope<'_, '_, v8::Context>, context: v8::Local<v8::Context>) {
        let fs = Self::create_object(scope);
        let global = context.global(scope);
        let Some(name) = v8::String::new(scope, "fs") else {
            return;
        };
        global.set(scope, name.into(), fs.into());
    }

    pub fn create_object<'s>(
        scope: &mut v8::PinScope<'s, '_, v8::Context>,
    ) -> v8::Local<'s, v8::Object> {
        let fs = v8::Object::new(scope);
        let read_name = v8::String::new(scope, "readFileSync").unwrap();
        let read = v8::FunctionTemplate::new(scope, read_file_sync)
            .get_function(scope)
            .unwrap();
        fs.set(scope, read_name.into(), read.into());

        let write_name = v8::String::new(scope, "writeFileSync").unwrap();
        let write = v8::FunctionTemplate::new(scope, write_file_sync)
            .get_function(scope)
            .unwrap();
        fs.set(scope, write_name.into(), write.into());

        let exists_name = v8::String::new(scope, "existsSync").unwrap();
        let exists = v8::FunctionTemplate::new(scope, exists_sync)
            .get_function(scope)
            .unwrap();
        fs.set(scope, exists_name.into(), exists.into());
        fs
    }
}

fn path_argument(
    scope: &mut v8::PinScope,
    arguments: &v8::FunctionCallbackArguments,
) -> Option<String> {
    if arguments.length() == 0 {
        v8_bridge::throw_error(scope, "filesystem operation expects a path");
        return None;
    }
    v8_bridge::to_rust_string(scope, arguments.get(0))
}

fn read_file_sync(
    scope: &mut v8::PinScope,
    arguments: v8::FunctionCallbackArguments,
    mut return_value: v8::ReturnValue,
) {
    let Some(path) = path_argument(scope, &arguments) else {
        return;
    };
    match fs::read_to_string(&path) {
        Ok(contents) => {
            if let Some(contents) = v8::String::new(scope, &contents) {
                return_value.set(contents.into());
            }
        }
        Err(error) => v8_bridge::throw_error(
            scope,
            &format!("cannot read {}: {error}", Path::new(&path).display()),
        ),
    }
}

fn write_file_sync(
    scope: &mut v8::PinScope,
    arguments: v8::FunctionCallbackArguments,
    _return_value: v8::ReturnValue,
) {
    let Some(path) = path_argument(scope, &arguments) else {
        return;
    };
    if arguments.length() < 2 {
        v8_bridge::throw_error(scope, "writeFileSync expects a path and contents");
        return;
    }
    let Some(contents) = v8_bridge::to_rust_string(scope, arguments.get(1)) else {
        v8_bridge::throw_error(scope, "file contents could not be converted to text");
        return;
    };
    if let Err(error) = fs::write(&path, contents) {
        v8_bridge::throw_error(
            scope,
            &format!("cannot write {}: {error}", Path::new(&path).display()),
        );
    }
}

fn exists_sync(
    scope: &mut v8::PinScope,
    arguments: v8::FunctionCallbackArguments,
    mut return_value: v8::ReturnValue,
) {
    let Some(path) = path_argument(scope, &arguments) else {
        return;
    };
    return_value.set(v8::Boolean::new(scope, Path::new(&path).exists()).into());
}
