#include <sys/epoll.h>
#include <functional>
#include <iostream>
#include <set>
using namespace std;


struct TimerNodeBase {
    time_t expire;
    int64_t id;
};

struct TimerNode : public TimerNodeBase {
    using Callback = std::function<void(const TimerNode &node)>;
    Callback func; 
};

bool operator< (const TimerNodeBase &lhd, const TimerNodeBase &rhd){
    if(lhd.expire < rhd.expire){
        return true;
    }else if(lhd.expire > rhd.expire){
        return false;
    }else {
        return lhd.id < rhd.id;
    }
}


class Timer {
public:
    static time_t GetTick() {
        auto sc = chrono::time_point_cast<chrono::milliseconds> (chrono::steady_clock::now());
        auto temp = chrono::duration_cast<chrono::milliseconds> (sc.time_since_epoch());
        return temp.count();
    }
    static int64_t GenID() {
        return ++gid;
    }
    // msec事件过后 ， 执行func
    TimerNodeBase AddTimer(time_t msec, TimerNode::Callback func){
        time_t expire = GetTick() + msec;
        if(timeouts.empty() || expire <= timeouts.crbegin()->expire){
            auto pairs = timeouts.emplace(GenID(), expire, std::move(func));
            return static_cast<TimerNodeBase>(*pairs.first);
        }
        auto ele = timeouts.emplace_hint(timeouts.crbegin().base(), GenID(), expire, std::move(func));
        return static_cast<TimerNodeBase>(*ele);
    }


    bool DelTimer(TimerNodeBase &node){
        // 加入到某个数据结构中 
        auto iter = timeouts.find(node); 
        if( iter != timeouts.end()){//找到 并删除
            timeouts.erase(iter);
            return true;
        }
        return false;
    }
    
    void HandleTimer(time_t now){
        auto iter =  timeouts.begin();
        while(iter != timeouts.end() && iter->expire <= now){
            iter->func(* iter);
            iter = timeouts.erase(iter);
        }
    }
    
    time_t CheckTimer(){
        // 从数据结构里取最小结点
        auto iter = timeouts.begin();
        if(iter == timeouts.end()){
            return -1;
        }
        time_t diss = iter->expire - GetTick();
        return diss > 0 ? diss : 0;
    }
    time_t TimeToSleep(){

    }
private:
    static int64_t gid;
    set <TimerNode, std::less<>> timeouts;

};
int64_t Timer::gid = 0;


int main()
{

    
}