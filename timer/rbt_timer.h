#ifndef RBT_TIMER
#define RBT_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <time.h>
#include "../log/log.h"
#include <set>

class util_timer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer
{
public:
    time_t expire;
    int64_t id;
    bool operator<(const util_timer &t) {
        if (expire != t.expire)
        {
            return expire < t.expire;
        }
        else
        {
            return id < t.id;
        }
    }
    void (* cb_func)(client_data *);
    client_data *user_data;
};

/*
bool operator<(const util_timer &a, const util_timer &b)
{
    if (a.expire != b.expire)
    {
        return a.expire < b.expire;
    }
    else
    {
        return a.id < b.id;
    }
}
*/

class sort_timer_rbt
{
public:
    sort_timer_rbt();
    ~sort_timer_rbt();

    static int64_t getid();
    void add_timer(util_timer *timer);
    bool del_timer(util_timer *timer);
    void tick();

private:
    set<util_timer*> timerset;
    static int64_t gid;
};

class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    sort_timer_rbt m_timer_rbt;
    static int u_epollfd;
    int m_TIMESLOT;
};

#endif

