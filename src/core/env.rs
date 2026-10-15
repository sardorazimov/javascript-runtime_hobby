// Copyright (c) 2026 Sardor Azimov
// SPDX-License-Identifier: MIT
//! Stores runtime paths and resolves entry-script paths against the working directory.

use std::path::{Path, PathBuf};

pub struct RuntimeEnvironment {
    working_directory: PathBuf,
}

impl RuntimeEnvironment {
    pub fn new(working_directory: PathBuf) -> Self {
        Self { working_directory }
    }

    pub fn resolve_script(&self, script: &Path) -> PathBuf {
        if script.is_absolute() {
            script.to_path_buf()
        } else {
            self.working_directory.join(script)
        }
    }
}
