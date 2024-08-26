#include "graphics/circular_span.hpp"

fi::graphics::CircularSpan::CircularSpan(void* data, size_t size)
{
    reference(data, size);
}

void fi::graphics::CircularSpan::reference(void* data, size_t size)
{
    begin_ = (std::byte*)data;
    end_ = (std::byte*)data + size;
}

bool fi::graphics::CircularSpan::push_back(void* data, size_t new_size)
{
    if (remainning() >= new_size)
    {
        size_t seek0 = (front_ + size_) % capacity();
        size_t seek0_size = std::min(new_size, capacity() - seek0);
        size_t seek1_size = new_size - seek0_size;
        memcpy(begin_ + seek0, data, seek0_size);
        if (seek1_size)
        {
            memcpy(begin_, (std::byte*)data + seek0_size, seek1_size);
        }

        size_ += new_size;
        blocks_.emplace(begin_ + seek0, new_size);
        return true;
    }
    return false;
}

void fi::graphics::CircularSpan::pop_front()
{
    if (size_)
    {
        size_ -= blocks_.front().second;
        front_ += blocks_.front().second;
        front_ %= capacity();
        blocks_.pop();
    }
}

void fi::graphics::CircularSpan::pop_front_cleared()
{
    std::pair<std::byte*, size_t>& block = blocks_.front();
    size_t seek0_size = std::min(block.second, capacity() - front_);
    size_t seek1_size = block.second - seek0_size;
    memset(block.first, 0x0, seek0_size);
    if (seek1_size)
    {
        memset(block.first + seek0_size, 0x0, seek0_size);
    }
    pop_front();
}

bool fi::graphics::CircularSpan::copy_front_to(std::byte* dst)
{
    if (size_)
    {
        std::pair<std::byte*, size_t>& block = blocks_.front();
        size_t seek0_size = std::min(block.second, capacity() - front_);
        size_t seek1_size = block.second - seek0_size;
        memcpy(dst, block.first, seek0_size);
        if (seek1_size)
        {
            memcpy(dst, block.first + seek0_size, seek1_size);
        }
        return true;
    }
    return false;
}

std::array<std::pair<size_t, size_t>, 2> fi::graphics::CircularSpan::front_region()
{
    std::pair<std::byte*, size_t>& block = blocks_.front();
    std::array<std::pair<size_t, size_t>, 2> seeks;
    seeks[0].first = front_;
    seeks[0].second = std::min(block.second, capacity() - front_);
    seeks[1].first = 0;
    seeks[1].second = block.second - seeks[0].second;
    return seeks;
}
