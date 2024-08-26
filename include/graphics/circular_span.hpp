#ifndef GRAPHICS_CIRCULAR_SPAN_HPP
#define GRAPHICS_CIRCULAR_SPAN_HPP

#include <queue>
#include <cstring>

namespace fi::graphics
{
    class CircularSpan
    {
      private:
        std::byte* begin_ = nullptr;
        std::byte* end_ = nullptr;
        size_t front_ = 0;
        size_t size_ = 0;
        std::queue<std::pair<std::byte*, size_t>> blocks_{};

      public:
        CircularSpan(void* data, size_t size);
        CircularSpan() = default;

        void reference(void* data, size_t size);
        bool push_back(void* data, size_t new_size);
        void pop_front();
        void pop_front_cleared();
        bool copy_front_to(std::byte* dst);
        size_t front_block_size() { return blocks_.front().second; }
        size_t front() { return front_; }
        size_t empty() { return !size_; }
        size_t size() { return size_; }
        size_t capacity() { return end_ - begin_; }
        size_t remainning() { return capacity() - size(); }
    };
}; // namespace fi::graphics

#endif // GRAPHICS_CIRCULAR_SPAN_HPP
