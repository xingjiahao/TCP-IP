#include <functional>

class Timer {
  private:
    size_t _time;
    bool _is_running;
  public:
    Timer():_time(0),_is_running(false) {}
    size_t time(){ return _time; }
    bool is_running(){ return _is_running; }
    void stop(){ _is_running=false; }
    void reset(){ _time=0; }
    void add_time(size_t t){ _time+=t; }
    void run(){ _is_running=true; }
};