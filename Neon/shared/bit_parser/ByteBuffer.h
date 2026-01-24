#ifndef SHARED_BIT_PARSER_BYTEBUFFER_H
#define SHARED_BIT_PARSER_BYTEBUFFER_H

#include <cstdint>
#include <vector>
#include <string>
#include <string_view>

#include <limits>

namespace shared::bit_parser
{
class ByteBuffer {
public:
    void clear() { data_.clear(); }
    void reserve(std::size_t n) { data_.reserve(n); }

    const uint8_t* data() const { return data_.data(); }
    uint8_t* data() { return data_.data(); }

    std::size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }

    const std::vector<uint8_t>& vec() const { return data_; }
    std::vector<uint8_t>& vec() { return data_; }

    void resize(std::size_t n, uint8_t fill = 0) { data_.resize(n, fill); }
    void push_back(uint8_t b) { data_.push_back(b); }

private:
    std::vector<uint8_t> data_;
};

}

#endif 