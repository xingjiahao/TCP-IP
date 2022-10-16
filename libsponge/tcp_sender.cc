#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

// template <typename... Targs>
// void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _rto(retx_timeout)
    , _alarm()
    , _retran_num(0)
    , _window_size(1)
    , _send_fin(false)
    , _ack_abs_seqno(0) {}

uint64_t TCPSender::bytes_in_flight() const {
    uint64_t size=0;
    size_t i=0;
    for(;i<_outstanding.size();i++){
        size+=_outstanding[i].length_in_sequence_space();
    }
    return size;
}

void TCPSender::fill_window() {
    if(_send_fin) return;
    size_t tmp_window_size= _window_size==0?1:_window_size;
    
    while(_next_seqno<_ack_abs_seqno+tmp_window_size){
        TCPSegment seg;
        seg.header().seqno=next_seqno();
        uint64_t tmp_next_seqno=_next_seqno;
        if(_next_seqno==0){
            seg.header().syn=true;
            _next_seqno++;
        }
        size_t len=min(_ack_abs_seqno+tmp_window_size-_next_seqno,TCPConfig::MAX_PAYLOAD_SIZE);
        string s=_stream.read(len);
        _next_seqno+=s.length();
        seg.payload() = Buffer(move(s));
        if(_stream.eof()&&(_ack_abs_seqno+tmp_window_size-_next_seqno>0)&&!_send_fin){
            seg.header().fin=true;
            _send_fin=true;
            _next_seqno++;
        }
        if(seg.length_in_sequence_space()==0) break;
        _segments_out.push(seg);
        _outstanding_seqno.push(tmp_next_seqno);
        _outstanding.push_back(seg);
        if(!_alarm.is_running()){
            _alarm.reset();
            _alarm.run();
        }
        if(_stream.buffer_empty()||_send_fin) break;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    if(_outstanding.empty()||unwrap(ackno,_isn,_outstanding_seqno.front())<_outstanding_seqno.front()) return;
    uint64_t t=_outstanding_seqno.front()+_outstanding.front().length_in_sequence_space();
    uint64_t abs_ackno=unwrap(ackno,_isn,_outstanding_seqno.front());
    
    if(_outstanding.front().header().syn&&abs_ackno!=t) return;

    while(abs_ackno>=t){
        _outstanding.pop_front();
        _outstanding_seqno.pop();
        _ack_abs_seqno=t;
        _rto=_initial_retransmission_timeout;
        _retran_num=0;
        if(!_outstanding.empty()){
            _alarm.reset();
            _alarm.run();
        }
        else{
            _alarm.stop();
            break;
        }
        t=_outstanding_seqno.front()+_outstanding.front().length_in_sequence_space();
    }
    _window_size=window_size;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(_alarm.is_running()){
        _alarm.add_time(ms_since_last_tick);
        if(_alarm.time()>=_rto){
            _segments_out.push(_outstanding.front());
            _retran_num++;
            if(_window_size>0){
                _rto*=2;
            }
            _alarm.reset();
        }
    } 
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retran_num; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno=next_seqno();
    _segments_out.push(seg);
}
