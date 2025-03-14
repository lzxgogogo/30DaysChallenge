#include <sys/epoll.h>
#include <functional>
#include <chrono>
#include <iostream>
#include <map>          // 改用 multimap
#include <memory>
using namespace std;

struct TimerNodeBase {
    time_t expire;
    int64_t id;
    // 需要提供 operator< 用于排序
    bool operator<(const TimerNodeBase& other) const {
        return (expire < other.expire) || (expire == other.expire && id < other.id);
    }
};

struct TimerNode : public TimerNodeBase {
    using Callback = std::function<void(const TimerNode &node)>;
    Callback func;

    // 构造函数初始化基类成员
    TimerNode(int64_t id, time_t expire, Callback func)
        : TimerNodeBase{expire, id}, func(std::move(func)) {}
};

class Timer {
public:
    static time_t GetTick() {
        auto sc = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
        return chrono::duration_cast<chrono::milliseconds>(sc.time_since_epoch()).count();
    }

    static int64_t GenID() {
        return ++gid;
    }

    // 添加定时器
    TimerNodeBase AddTimer(time_t msec, TimerNode::Callback func) {
        time_t expire = GetTick() + msec;
        int64_t id = GenID();
        TimerNode node(id, expire, std::move(func));
        // 插入到 multimap（以 expire 为 key）
        auto iter = timeouts.emplace(expire, std::move(node));
        return static_cast<TimerNodeBase>(iter->second);
    }

    // 删除定时器
    bool DelTimer(TimerNodeBase &node) {
        // 查找相同 expire 的范围
        auto range = timeouts.equal_range(node.expire);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second.id == node.id) {
                timeouts.erase(it);
                return true;
            }
        }
        return false;
    }

    // 处理到期定时器
    void HandleTimer(time_t now) {
        auto iter = timeouts.begin();
        while (iter != timeouts.end() && iter->first <= now) {
            iter->second.func(iter->second); // 触发回调
            iter = timeouts.erase(iter);
        }
    }

    // 计算下次等待时间
    time_t TimeToSleep() {
        if (timeouts.empty()) return -1;
        time_t diss = timeouts.begin()->first - GetTick();
        return diss > 0 ? diss : 0;
    }

private:
    static int64_t gid;
    multimap<time_t, TimerNode> timeouts; // 改用 multimap
};

int64_t Timer::gid = 0;


int main()
{
    int epfd = epoll_create(1);
    
    unique_ptr<Timer> timer = make_unique<Timer>();
    
    int i = 0;
    timer->AddTimer(1000, [&](const TimerNode &node){
        cout << Timer::GetTick() << " node id:" << node.id << " revoked times:" << ++i <<endl;
    });
    
    timer->AddTimer(1000, [&](const TimerNode &node) {
        cout << Timer::GetTick() << " node id:" << node.id << " revoked times:" << ++i << endl;
    });

    timer->AddTimer(3000, [&](const TimerNode &node) {
        cout << Timer::GetTick() << " node id:" << node.id << " revoked times:" << ++i << endl;
    });

    auto node = timer->AddTimer(2100, [&](const TimerNode &node) {
        cout << Timer::GetTick() << " node id:" << node.id << " revoked times:" << ++i << endl;
    });

    timer->DelTimer(node);

    cout << "now time:" << Timer::GetTick() << endl;
    epoll_event ev[64] = {0};

    while (true) {

        int n = epoll_wait(epfd, ev, 64, timer->TimeToSleep());
        time_t now = Timer::GetTick();
        for (int i = 0; i < n; i++) {
            /**/
        }
        /* 处理定时事件*/
        timer->HandleTimer(now);
    }
    

    return 0;
}