// Copyright (c) 2026 Sardor Azimov
// SPDX-License-Identifier: MIT
//! Command-line entry point for the independent Rust V8 runtime experiment.

mod binding;
mod core;
mod modules;

use std::path::PathBuf;

use core::runtime::Runtime;

fn main() {
    let script = std::env::args_os()
        .nth(1)
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from("rust-app.js"));

    let working_directory = match std::env::current_dir() {
        Ok(path) => path,
        Err(error) => {
            eprintln!("MiniRust: cannot read current directory: {error}");
            std::process::exit(1);
        }
    };

    let mut runtime = match Runtime::new(working_directory) {
        Ok(runtime) => runtime,
        Err(error) => {
            eprintln!("MiniRust: {error}");
            std::process::exit(1);
        }
    };

    if let Err(error) = runtime.run_file(&script) {
        eprintln!("MiniRust: {error}");
        std::process::exit(1);
    }
}
