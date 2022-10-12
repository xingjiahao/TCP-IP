#include "stream_reassembler.hh"
#include<iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
_buffer(capacity,byte{'\0',0,false}),
_first_unassembled(0),
// _first_unacceptable(capacity),
_unassembled_bytes(0),
_end(false),
_output(capacity), 
_capacity(capacity)
{}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);
    if(eof&&index+data.length()<=_output.bytes_read()+_capacity){
        _end=true;
    }
    if(data==""){
        if(empty()){
            _output.end_input();
        }
        return;
    }
    if(index>=_output.bytes_read()+_capacity||index+data.length()<_first_unassembled){
        return;
    }

    size_t data_begin_index=max(index,_first_unassembled);
    size_t data_end_index=min(index+data.length()-1,_output.bytes_read()+_capacity-1);
    string s=data.substr(data_begin_index-index,data_end_index-data_begin_index+1);
    for(size_t i=0;i<s.length();i++){
        if(_buffer[data_begin_index-_first_unassembled+i].val=='\0'){
            _buffer[data_begin_index-_first_unassembled+i]=byte{s[i],i+data_begin_index,true};
            _unassembled_bytes++;
        }
    }
    if(_first_unassembled==_buffer.front().index&&_buffer.front().is_write==true){
        string r="";
        while(_first_unassembled==_buffer.front().index&&_buffer.front().is_write==true){
            r+=_buffer.front().val;
            _first_unassembled++;
            _buffer.pop_front();
            _buffer.push_back(byte{'\0',0,false});
            _unassembled_bytes--;
        }
        _output.write(r);
    }
    if(empty()){
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    return _unassembled_bytes;
}

bool StreamReassembler::empty() const {
    return _end&&(_unassembled_bytes==0);
}
