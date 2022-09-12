#include "rbt_timer.h"
#include <memory>
#include <iostream>

int main() {
    int epfd = epoll_create(1);
    epoll_event ev[64] = {0};

    unique_ptr<sort_timer_rbt> timer = make_unique<sort_timer_rbt>();
    util_timer *node;
    node->id = 1;
    node->expire = 1;
    timer->add_timer(node);

    while (true) {
        int n = epoll_wait(epfd, ev, 64, -1);
        for (int i = 0; i < n; i++) {
            cout << i << endl;
        }
    }
    return 0;
}


