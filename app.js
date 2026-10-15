const fs = require("fs");
const Buffer = require("buffer");

console.log("MiniNode V8 runtime başladı");
console.log("Çalışma dizini:", __dirname);

const message = Buffer.from("Learning Node concepts with C++.");
console.log("Buffer:", message.toString());
console.log("Buffer mı?", Buffer.isBuffer(message));

fs.writeFileSync("mini-node-output.txt", message.toString());
console.log("Dosya yazıldı mı?", fs.existsSync("mini-node-output.txt"));
fs.readFile("mini-node-output.txt", (error, contents) => {
	if (error) {
		console.error(error.message);
		return;
	}
	console.log("Asenkron dosya okuma:", contents);
});

setTimeout(() => {
	console.log("Timer callback çalıştı");
}, 25);