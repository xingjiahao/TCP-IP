#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(seg.header().syn){
        _is_setISN=true;
        _ISN=seg.header().seqno;
    }
    else{
        if(_is_setISN==false) return;
    }
    uint64_t index=unwrap(seg.header().seqno,_ISN,_reassembler.stream_out().bytes_written());
    string s=seg.payload().copy();
    if(seg.header().syn){
        _reassembler.push_substring(s,index,seg.header().fin);
    }
    else{
        _reassembler.push_substring(s,index-1,seg.header().fin);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(_is_setISN){
        if(_reassembler.stream_out().input_ended()) 
            return wrap(_reassembler.stream_out().bytes_written()+2,_ISN);
        else
            return wrap(_reassembler.stream_out().bytes_written()+1,_ISN);
    }
    return nullopt;
}

size_t TCPReceiver::window_size() const {
    return  _capacity
            -_reassembler.stream_out().bytes_written()
            +_reassembler.stream_out().bytes_read();
}
