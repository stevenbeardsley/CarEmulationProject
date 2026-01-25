#ifndef SHARED_BIT_PARSER_BYTEBUFFER_H
#define SHARED_BIT_PARSER_BYTEBUFFER_H

namespace shared::bit_parser
{

class ByteBuffer {
public:
    explicit ByteBuffer(std::vector<uint8_t>& backing)
        : dataRef_(&backing) {
    }

    void clear() { dataRef_->clear(); }
    void reserve(std::size_t n) { dataRef_->reserve(n); }

    const uint8_t* data() const { return dataRef_->data(); }
    uint8_t* data() { return dataRef_->data(); }

    std::size_t size() const { return dataRef_->size(); }
    bool empty() const { return dataRef_->empty(); }

    const std::vector<uint8_t>& vec() const { return *dataRef_; }
    std::vector<uint8_t>& vec() { return *dataRef_; }

    void resize(std::size_t n, uint8_t fill = 0) { dataRef_->resize(n, fill); }
    void push_back(uint8_t b) { dataRef_->push_back(b); }

private:
    std::vector<uint8_t>* dataRef_;
};

}
#endif 