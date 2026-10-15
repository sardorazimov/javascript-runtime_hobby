#pragma once

#include <cstdint>

#include <v8.h>

namespace mininode {

class GarbageCollectorBridge {
public:
   explicit GarbageCollectorBridge(v8::Isolate* isolate) : isolate_(isolate) {}

   void AdjustExternalMemory(std::int64_t delta_bytes) {
      if (delta_bytes == 0) {
         return;
      }
      tracked_bytes_ += delta_bytes;
      isolate_->AdjustAmountOfExternalAllocatedMemory(delta_bytes);
   }

   std::int64_t tracked_bytes() const noexcept {
      return tracked_bytes_;
   }

private:
   v8::Isolate* isolate_;
   std::int64_t tracked_bytes_ = 0;
};

}