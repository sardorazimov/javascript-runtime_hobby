const http = require("http");
const tls = require("tls");
const socket = require("socket");

const httpResponse = http.getSync("http://example.com");
console.log("HTTP status:", httpResponse.statusCode);
console.log("HTTP response bytes:", httpResponse.body.length);

const tlsResponse = tls.getSync("https://example.com");
console.log("TLS/HTTPS status:", tlsResponse.statusCode);
console.log("TLS/HTTPS response bytes:", tlsResponse.body.length);

const socketResponse = socket.requestSync(
    "example.com",
    80,
    "GET / HTTP/1.0\r\nHost: example.com\r\n\r\n"
);
console.log("Raw socket response:", socketResponse.slice(0, 15));
