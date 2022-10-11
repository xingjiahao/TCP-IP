#include "stream_reassembler.hh"
#include<iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
_output(capacity), 
_capacity(capacity),
_buffer(),
_index(),
_expect_index(0),
_size(0),
_end(false) 
{}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    if(index<_expect_index) return;
    size_t len=min(_capacity-_size-_output.buffer_size(),data.length());
    const string s=data.substr(0,len);
    size_t i=0;
    for(;i<_index.size()&&_index[i]<index;i++);
    if(i==_index.size()){
        _buffer.push_back(s);
        _index.push_back(index);
        _size+=len;
    }
    else{
        if(index==_index[i]){
            return;
        }
        else{
            _buffer.insert(_buffer.begin()+(long)i,s);
            _index.insert(_index.begin()+(long)i,index);
            _size+=len;
        }
    }
    if(eof){
        _end=eof;
    }
    // std::cout<<"The error is this:!!!!"<<std::endl;
    // std::cout<<_buffer.front()<<" "<<_index.front()<<" "<<_expect_index<<" "<<_size<<std::endl;
    while(!_buffer.empty()&& _expect_index==_index.front()){
        _output.write(_buffer.front());
        _expect_index+=_buffer.front().length();
        _size-=_buffer.front().length();
        _buffer.pop_front();
        _index.pop_front();

    }
}

size_t StreamReassembler::unassembled_bytes() const {
    size_t t=0;
    for(size_t i=0;i<_buffer.size();i++){
        t+=_buffer[i].length();
    }
    return t;
}

bool StreamReassembler::empty() const {
    return _end&&_buffer.empty();
}
