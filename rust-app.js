const fs = require("fs");
const Buffer = require("buffer");

console.log("MiniRust V8 runtime started");
console.log("Native console arguments:", 42, true);
console.log("before timer");

fs.writeFileSync("rust-runtime-output.txt", "Rust owns this file operation.");
console.log("File exists:", fs.existsSync("rust-runtime-output.txt"));
console.log("File contents:", fs.readFileSync("rust-runtime-output.txt"));

const message = Buffer.from("Rust buffer bytes");
console.log("Buffer:", message.toString());
console.log("Is Buffer:", Buffer.isBuffer(message));
console.log("Allocated byte length:", Buffer.alloc(8).byteLength);

const cancelledTimer = setTimeout(() => console.log("should not print"), 0);
clearTimeout(cancelledTimer);
setTimeout(() => console.log("timer callback"), 10);
console.log("after timer scheduling");