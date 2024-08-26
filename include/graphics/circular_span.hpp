#ifndef GRAPHICS_CIRCULAR_SPAN_HPP
#define GRAPHICS_CIRCULAR_SPAN_HPP

#include <queue>
#include <cstring>

#include <vulkan/vulkan.hpp>

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
        CircularSpan() = default;
        CircularSpan(void* data, size_t size);
        template <typename T>
        CircularSpan(const T& continues_container)
            : CircularSpan(continues_container.data(), sizeof(continues_container[0]) * continues_container.size())
        {
        }

        void reference(void* data, size_t size);
        bool push_back(void* data, size_t new_size);
        void pop_front();
        void pop_front_cleared();
        bool copy_front_block_to(std::byte* dst);
        std::array<std::pair<size_t, size_t>, 2> front_block_region();
        size_t front_block_size() { return blocks_.front().second; }
        size_t front() { return front_; }
        size_t empty() { return !size_; }
        size_t size() { return size_; }
        size_t capacity() { return end_ - begin_; }
        size_t remainning() { return capacity() - size(); }
    };
}; // namespace fi::graphics

#endif // GRAPHICS_CIRCULAR_SPAN_HPP
