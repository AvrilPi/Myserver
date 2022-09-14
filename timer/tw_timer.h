#ifndef TW_TIMER
#define TW_TIMER

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
    char buf[128];
    util_timer *timer;
};

class util_timer
{
public:
    util_timer(int rot, int ts):next(NULL), prev(NULL), rotation(rot), time_slot(ts){}

public:
    int rotation; // 记录定时器在时间轮转多少圈后生效
    int time_slot; // 记录定时器属于时间轮上哪个槽对应的链表
    
    client_data *user_data;
    void (* cb_func)(client_data *);
    util_timer* next; // 指向下一个定时器
    util_timer* prev; // 指向前一个定时器
};

class sort_timer_tw
{
public:
    sort_timer_tw();
    ~sort_timer_tw();

    util_timer* add_timer(int expire, client_data *user_data, void (*cb_func)(client_data *));
    void adjust_timer(int expire, util_timer *timer);
    bool del_timer(util_timer *timer);
    void tick();

private:
    static const int N = 120; // 时间轮上槽的数目
    static const int SI = 1; // 每1s时间轮转动一次，即槽间隔为1s；

    util_timer* slots[N];

    int cur_slot; // 时间轮的当前槽
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
    sort_timer_tw m_timer_tw;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);

#endif

