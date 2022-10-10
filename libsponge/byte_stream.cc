#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) :_capacity_size(capacity),_written_size(0),_read_size(0),_end_input(false),_error(false) {}

size_t ByteStream::write(const string &data) {
    size_t i=0;
    for(;i<data.length()&&stream.size()<_capacity_size;i++){
        stream.push_back(data[i]);
        _written_size++;
    }
    return i;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t length=min(len,stream.size());
    return string(stream.begin(),stream.begin()+length);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t num=min(len,stream.size());
    for(size_t i=0;i<num;i++){
        stream.pop_front();
        _read_size++;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string s=peek_output(len);
    pop_output(len);
    return s;
}

void ByteStream::end_input() {
    _end_input=true;
}

bool ByteStream::input_ended() const {
    return _end_input;
}

size_t ByteStream::buffer_size() const {
    return stream.size();
}

bool ByteStream::buffer_empty() const {
    return stream.empty();
}

bool ByteStream::eof() const {
    return _end_input&&stream.empty();
}

size_t ByteStream::bytes_written() const {
    return _written_size;
}

size_t ByteStream::bytes_read() const {
    return _read_size;
}

size_t ByteStream::remaining_capacity() const {
    return _capacity_size-stream.size();
}
