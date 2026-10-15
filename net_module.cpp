#include "net_module.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <string>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "binding.h"

namespace mininode::NetModule {
namespace {

constexpr std::size_t kMaximumResponseSize = 1024 * 1024;

void ThrowError(v8::Isolate* isolate, const std::string& message) {
   isolate->ThrowException(v8::Exception::Error(ToV8String(isolate, message)));
}

void TcpRequestSync(const v8::FunctionCallbackInfo<v8::Value>& info) {
   auto* isolate = info.GetIsolate();
   if (info.Length() < 3 || !info[1]->IsInt32()) {
      isolate->ThrowException(v8::Exception::TypeError(ToV8String(
         isolate, "tcpRequestSync expects host, port, and request text")));
      return;
   }

   const auto host = ToStdString(isolate, info[0]);
   const int port = info[1]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
   if (host.empty() || port < 1 || port > 65535) {
      isolate->ThrowException(v8::Exception::RangeError(
         ToV8String(isolate, "Invalid host or TCP port")));
      return;
   }
   const auto request = ToStdString(isolate, info[2]);

   addrinfo hints{};
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   addrinfo* addresses = nullptr;
   const auto service = std::to_string(port);
   const int lookup = getaddrinfo(host.c_str(), service.c_str(), &hints, &addresses);
   if (lookup != 0) {
      ThrowError(isolate, std::string("DNS lookup failed: ") + gai_strerror(lookup));
      return;
   }

   int socket_fd = -1;
   for (auto* address = addresses; address != nullptr; address = address->ai_next) {
      socket_fd = socket(address->ai_family, address->ai_socktype,
                     address->ai_protocol);
      if (socket_fd >= 0 &&
         connect(socket_fd, address->ai_addr, address->ai_addrlen) == 0) {
         break;
      }
      if (socket_fd >= 0) {
         close(socket_fd);
         socket_fd = -1;
      }
   }
   freeaddrinfo(addresses);
   if (socket_fd < 0) {
      ThrowError(isolate, std::string("TCP connection failed: ") +
                        std::strerror(errno));
      return;
   }

   int no_sigpipe = 1;
   setsockopt(socket_fd, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe,
            sizeof(no_sigpipe));
   timeval timeout{2, 0};
   setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

   std::size_t sent = 0;
   while (sent < request.size()) {
      const auto count = send(socket_fd, request.data() + sent,
                        request.size() - sent, 0);
      if (count <= 0) {
         const auto error = std::string("TCP send failed: ") +
                        std::strerror(errno);
         close(socket_fd);
         ThrowError(isolate, error);
         return;
      }
      sent += static_cast<std::size_t>(count);
   }

   std::string response;
   std::array<char, 4096> buffer{};
   while (response.size() < kMaximumResponseSize) {
      const auto count = recv(socket_fd, buffer.data(), buffer.size(), 0);
      if (count == 0) {
         break;
      }
      if (count < 0) {
         if (errno == EINTR) {
            continue;
         }
         if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
         }
         const auto error = std::string("TCP receive failed: ") +
                        std::strerror(errno);
         close(socket_fd);
         ThrowError(isolate, error);
         return;
      }
      const auto available = kMaximumResponseSize - response.size();
      response.append(buffer.data(),
            std::min(static_cast<std::size_t>(count), available));
   }
   close(socket_fd);
   info.GetReturnValue().Set(ToV8String(isolate, response));
}

}

v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context) {
   auto* isolate = context->GetIsolate();
   auto net = v8::Object::New(isolate);
   net->Set(context, ToV8String(isolate, "tcpRequestSync"),
          v8::Function::New(context, TcpRequestSync).ToLocalChecked())
      .Check();
   return net;
}

void Install(v8::Isolate*, v8::Local<v8::Context> context) {
   auto net = CreateObject(context);
   context->Global()->Set(context, ToV8String(context->GetIsolate(), "net"), net)
      .Check();
}

}