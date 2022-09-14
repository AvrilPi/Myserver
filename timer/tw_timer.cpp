#include "tw_timer.h"
#include "../http/http_conn.h"

sort_timer_tw::sort_timer_tw()
{
    cur_slot = 0;
    for (int i = 0; i < N; ++i) {
        slots[i] = NULL;
    }
}

sort_timer_tw::~sort_timer_tw()
{
    for (int i = 0; i < N; ++i) 
    {
        util_timer* tmp = slots[i];
        while (tmp)
        {
            slots[i] = tmp->next;
            delete tmp;
            tmp = slots[i];
        }
    }
}

util_timer* sort_timer_tw::add_timer(int expire, client_data* user_data, void (*cb_func)(client_data *))
{
    /*
    if (!timer)
    {
        return;
    } 
    */
    if (expire < 0)
    {
	return NULL;
    }
    int ticks = 0;
    // 根据待插入定时器的超时值计算它将在时间轮转动多少个滴答后被触发，并将该滴答数存储于变量ticks中。
    // 如果待插入定时器的超时值小于时间轮的槽间隔SI，则将ticks向上折合为1，否则就将ticks向下折合为timeout / SI。
    if (expire < SI)
    {
        ticks = 1;
    }
    else
    {
        ticks = expire / SI;
    }
    // 计算待插入的定时器在时间轮转动多少圈后被触发
    int rotation = ticks / N;
    // 计算待插入的定时器应该被插入哪个槽中
    int ts = (cur_slot + (ticks % N)) % N;
    // 创建新的定时器，它在时间轮转动rotation圈之后被触发，且位于第第ts个槽上
    util_timer* timer = new util_timer(rotation, ts);
    // 如果第ts个槽中尚无任何定时器，则把新建的定时器插入其中，并将该定时器设置为该槽的头节点
    timer->user_data = user_data;
    timer->cb_func = cb_func;
    if (!slots[ts])
    {
        slots[ts] = timer;
    }
    // 否则，将定时器插入第ts个槽中
    else
    {
        timer->next = slots[ts];
        slots[ts]->prev = timer;
        slots[ts] = timer;
    }
}

void sort_timer_tw::adjust_timer(int expire, util_timer* timer)
{
    if (!timer)
    {
	return;
    }
    int ticks = 0;
    if (expire < SI)
    {
	ticks = 1;
    }
    else
    {
	ticks = expire / SI;
    }
    int rotation = ticks / N;
    int ts = (cur_slot + (ticks % N)) % N;
    if (timer->next == NULL)
    {
	 if (timer == slots[timer->time_slot])
	 {
	     slots[timer->time_slot] = slots[timer->time_slot]->next;
             slots[timer->time_slot]->prev = NULL;
             timer->next = NULL;
	 }
	 else
	 {
	     timer->prev->next = timer->next;
	 }
    }
    else
    {
         if (timer == slots[timer->time_slot])
         {
	      slots[timer->time_slot] = slots[timer->time_slot]->next;
 	      slots[timer->time_slot]->prev = NULL;
	      timer->next = NULL;
         }
         else
         {
	      timer->prev->next = timer->next;
	      timer->next->prev = timer->prev;
         }
    }
    timer->rotation = rotation;
    timer->time_slot = ts;
    if (!slots[ts])
    {
	slots[ts] = timer;
    }
    else
    {
	timer->next = slots[ts];
	slots[ts]->prev = timer;
	slots[ts] = timer;
    }
}

bool sort_timer_tw::del_timer(util_timer* timer)
{
    if (!timer)
    {
        return false;
    }
    int ts = timer->time_slot;
    // slots[ts]是目标定时器所在槽的头节点。如果目标定时器就是该头节点，则需要重置第ts个槽的头节点
    if (timer == slots[ts])
    {
        slots[ts] = slots[ts]->next;
        if (slots[ts])
        {
            slots[ts]->prev = NULL;
        }
        delete timer;
    }
    else
    {
        timer->prev->next = timer->next;
        if (timer->next)
        {
            timer->next->prev = timer->prev;
        }
        delete timer;
    }
    return true;
}

// SI时间到后，调用该函数，时间轮向前滚动一个槽的间隔

void sort_timer_tw::tick()
{
    util_timer *tmp = slots[cur_slot]; // 取得时间轮上当前槽的头节点
    while (tmp)
    {
        if (tmp->rotation > 0)
        {
            tmp->rotation--;
            tmp = tmp->next;
        }
        // 否则，说明定时器已经到期，于是执行定时任务，然后删除定时器
        else
        {
            tmp->cb_func(tmp->user_data);
            if (tmp == slots[cur_slot])
            {
                slots[cur_slot] = tmp->next;
                delete tmp;
                if (slots[cur_slot])
                {
                    slots[cur_slot]->prev = NULL;
                }
                tmp = slots[cur_slot];
            }
            else
            {
                tmp->prev->next = tmp->next;
                if (tmp->next)
                {
                    tmp->next->prev = tmp->prev;
                }
                util_timer *tmp2 = tmp->next;
                delete tmp;
                tmp = tmp2;
            }
        }
    }
    cur_slot = ++cur_slot % N; // 更新时间轮的当前槽，以反映时间轮的转动
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
    m_timer_tw.tick();
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






