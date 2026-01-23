
namespace shared::bit_parser
{

template<typename T>
class Span {
public:
    using element_type = T;
    using pointer = const T*;
    using iterator = const T*;

    Span(const T* data, size_t size) : data_(data), size_(size) {}

    const T* data() const { return data_; }
    size_t size() const { return size_; }

    const T& operator[](size_t i) const { return data_[i]; }

    iterator begin() const { return data_; }
    iterator end() const { return data_ + size_; }

private:
    const T* data_;
    size_t size_;
};

}