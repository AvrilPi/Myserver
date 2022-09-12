#include "rbt_timer.h"
#include "../http/http_conn.h"

sort_timer_rbt::sort_timer_rbt()
{
    timerset.clear();
}
int64_t sort_timer_rbt::gid = 0;

sort_timer_rbt::~sort_timer_rbt()
{
    auto iter = timerset.begin();
    while (iter != timerset.end())
    {
        auto tmp = iter;
        timerset.erase(tmp);
        iter++;
    }
}

int64_t sort_timer_rbt:: getid() {
    return ++gid;
}

void sort_timer_rbt::add_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    timer->id = getid();
    timerset.insert(timer);
}

bool sort_timer_rbt::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return false;
    }
    auto iter = timerset.find(timer);
    if (iter != timerset.end()) {
        timerset.erase(iter);
        return true;
    }
    return false;
}

void sort_timer_rbt::tick()
{
    if (timerset.empty())
    {
        return;
    }

    time_t cur = time(NULL);
    auto iter = timerset.begin();
    while (iter != timerset.end())
    {
        if (cur < (*iter)->expire)
        {
            break;
        }
        (*iter)->cb_func((*iter)->user_data);
        auto tmp = iter;
        timerset.erase(tmp);
        iter++;
    }
}

void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_rbt.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}
