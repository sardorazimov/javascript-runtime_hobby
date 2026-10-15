#include "buffer.h"

#include <algorithm>
#include <cstddef>
#include <string>

#include "binding.h"

namespace mininode::BufferModule {
namespace {

constexpr std::size_t kMaximumBufferSize = 64 * 1024 * 1024;

void BufferToString(const v8::FunctionCallbackInfo<v8::Value>& info) {
   auto* isolate = info.GetIsolate();
   if (!info.This()->IsUint8Array()) {
      isolate->ThrowException(v8::Exception::TypeError(
         ToV8String(isolate, "Buffer.toString called on an incompatible value")));
      return;
   }

   auto bytes = info.This().As<v8::Uint8Array>();
   if (bytes->ByteLength() == 0) {
      info.GetReturnValue().Set(ToV8String(isolate, ""));
      return;
   }
   auto backing_store = bytes->Buffer()->GetBackingStore();
   const auto* data = static_cast<const char*>(backing_store->Data()) +
                  bytes->ByteOffset();
   info.GetReturnValue().Set(ToV8String(
      isolate, std::string_view(data, bytes->ByteLength())));
}

v8::Local<v8::Uint8Array> MakeBuffer(v8::Local<v8::Context> context,
                            const char* data, std::size_t size) {
   auto* isolate = context->GetIsolate();
   auto array_buffer = v8::ArrayBuffer::New(isolate, size);
   auto result = v8::Uint8Array::New(array_buffer, 0, size);
   if (size > 0) {
      auto backing_store = array_buffer->GetBackingStore();
      std::copy_n(data, size, static_cast<char*>(backing_store->Data()));
   }
   auto to_string = v8::Function::New(context, BufferToString).ToLocalChecked();
   result->Set(context, ToV8String(isolate, "toString"), to_string).Check();
   result->Set(context, ToV8String(isolate, "_miniBuffer"),
            v8::True(isolate))
      .Check();
   return result;
}

void Alloc(const v8::FunctionCallbackInfo<v8::Value>& info) {
   auto* isolate = info.GetIsolate();
   if (info.Length() == 0 || !info[0]->IsNumber()) {
      isolate->ThrowException(v8::Exception::TypeError(
         ToV8String(isolate, "Buffer.alloc expects a byte length")));
      return;
   }
   const double requested = info[0]->NumberValue(isolate->GetCurrentContext())
                         .FromMaybe(-1);
   if (requested < 0 || requested > kMaximumBufferSize) {
      isolate->ThrowException(v8::Exception::RangeError(
         ToV8String(isolate, "Buffer length must be between 0 and 64 MiB")));
      return;
   }
   const auto size = static_cast<std::size_t>(requested);
   std::string bytes(size, '\0');
   info.GetReturnValue().Set(MakeBuffer(isolate->GetCurrentContext(),
                               bytes.data(), bytes.size()));
}

void From(const v8::FunctionCallbackInfo<v8::Value>& info) {
   auto* isolate = info.GetIsolate();
   if (info.Length() == 0) {
      isolate->ThrowException(v8::Exception::TypeError(
         ToV8String(isolate, "Buffer.from expects a string or Uint8Array")));
      return;
   }

   if (info[0]->IsString()) {
      const auto text = ToStdString(isolate, info[0]);
      info.GetReturnValue().Set(MakeBuffer(isolate->GetCurrentContext(),
                                  text.data(), text.size()));
      return;
   }
   if (!info[0]->IsUint8Array()) {
      isolate->ThrowException(v8::Exception::TypeError(
         ToV8String(isolate, "Buffer.from supports strings and Uint8Array")));
      return;
   }

   auto input = info[0].As<v8::Uint8Array>();
   const auto size = input->ByteLength();
   if (size > kMaximumBufferSize) {
      isolate->ThrowException(v8::Exception::RangeError(
         ToV8String(isolate, "Buffer length exceeds 64 MiB")));
      return;
   }
   auto backing_store = input->Buffer()->GetBackingStore();
   const auto* data = size == 0
                     ? ""
                     : static_cast<const char*>(backing_store->Data()) +
                         input->ByteOffset();
   info.GetReturnValue().Set(
      MakeBuffer(isolate->GetCurrentContext(), data, size));
}

void IsBuffer(const v8::FunctionCallbackInfo<v8::Value>& info) {
   if (info.Length() == 0 || !info[0]->IsObject()) {
      info.GetReturnValue().Set(false);
      return;
   }
   auto context = info.GetIsolate()->GetCurrentContext();
   auto result = info[0].As<v8::Object>()->Get(
      context, ToV8String(info.GetIsolate(), "_miniBuffer"));
   info.GetReturnValue().Set(
      !result.IsEmpty() && result.ToLocalChecked()->IsTrue());
}

}

v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context) {
   auto* isolate = context->GetIsolate();
   auto buffer = v8::Object::New(isolate);
   buffer->Set(context, ToV8String(isolate, "alloc"),
            v8::Function::New(context, Alloc).ToLocalChecked())
      .Check();
   buffer->Set(context, ToV8String(isolate, "from"),
            v8::Function::New(context, From).ToLocalChecked())
      .Check();
   buffer->Set(context, ToV8String(isolate, "isBuffer"),
            v8::Function::New(context, IsBuffer).ToLocalChecked())
      .Check();
   return buffer;
}

void Install(v8::Isolate*, v8::Local<v8::Context> context) {
   auto buffer = CreateObject(context);
   context->Global()->Set(context, ToV8String(context->GetIsolate(), "Buffer"),
                     buffer)
      .Check();
}

}