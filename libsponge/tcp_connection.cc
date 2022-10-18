#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPConnection::send_rst(){
    if(_sender.segments_out().empty()){
        TCPSegment seg;
        seg.header().rst=true;
        _segments_out.push(seg);
    }
    else{
        TCPSegment seg=_sender.segments_out().front();
        seg.header().rst=true;
        _segments_out.push(seg);
        _sender.segments_out().pop();
    }
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _linger_after_streams_finish=false;
    _is_active=false;
}

void TCPConnection::send_segment(){
    while(!_sender.segments_out().empty()){
        TCPSegment seg=_sender.segments_out().front();
        seg.header().win=_receiver.window_size();
        if(_receiver.ackno().has_value()){
            seg.header().ack=true;
            seg.header().ackno=_receiver.ackno().value();
        }
        _sender.segments_out().pop();
        _segments_out.push(seg);
    }
}

size_t TCPConnection::remaining_outbound_capacity() const {
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const {
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    return _time_since_last_segment;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_since_last_segment=0;

    if(seg.header().rst){
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        _linger_after_streams_finish=false;
        _is_active=false;
        return;
    }

    _receiver.segment_received(seg);
    if(!_receiver.ackno().has_value()) return;

    if (_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0)
    && seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
        send_segment();
        return;
    }

    if(seg.header().ack){
        WrappingInt32 ackno=seg.header().ackno;
        size_t window_size=seg.header().win;
        _sender.ack_received(ackno,window_size);
    }
    else{
        _sender.fill_window();
    }

    if(TCPState::state_summary(_receiver)==TCPReceiverStateSummary::FIN_RECV
       &&TCPState::state_summary(_sender)==TCPSenderStateSummary::SYN_ACKED)
    {
        _linger_after_streams_finish=false;
    }

    if(TCPState::state_summary(_receiver)==TCPReceiverStateSummary::FIN_RECV
       &&TCPState::state_summary(_sender)==TCPSenderStateSummary::FIN_ACKED)
    {
        if(!_linger_after_streams_finish){
            _is_active=false;
        }
    }

    if(seg.length_in_sequence_space()>0){
        if(_sender.segments_out().empty()){
            if(_sender.stream_in().buffer_empty())
                _sender.send_empty_segment();
            else
                _sender.fill_window();
        }
        if(_sender.segments_out().empty())
            _sender.send_empty_segment();
    }
    send_segment();
}

bool TCPConnection::active() const {
    return _is_active;
}

size_t TCPConnection::write(const string &data) {
    size_t write_size=_sender.stream_in().write(data);
    _sender.fill_window();
    send_segment();
    return write_size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment+=ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if(_sender.consecutive_retransmissions()>TCPConfig::MAX_RETX_ATTEMPTS){
        if(_sender.segments_out().empty())
            send_rst();
        else{
            _sender.segments_out().front().header().rst=true;
            _receiver.stream_out().set_error();
            _sender.stream_in().set_error();
            _linger_after_streams_finish=false;
            _is_active=false;
        }
    }
    send_segment();

    if(TCPState::state_summary(_receiver)==TCPReceiverStateSummary::FIN_RECV
       &&TCPState::state_summary(_sender)==TCPSenderStateSummary::FIN_ACKED
       &&_linger_after_streams_finish
       &&_time_since_last_segment>=10 * _cfg.rt_timeout)
    {
        _is_active=false;
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segment();
}

void TCPConnection::connect() {
    _sender.fill_window();
    send_segment();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            send_rst();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
