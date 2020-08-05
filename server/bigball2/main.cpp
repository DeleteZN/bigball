#include "tools.h"
#include <stdio.h>
#include "threadpool.h"
#include <semaphore.h>
#include <iostream>
#include <signal.h>
//字节对齐
//窗口大小
int windows_width = 800;
int windows_hight = 700;
//房间信息
static int room[10] = { 0,0,0,0,0,0,0,0,0 ,0 };

//创建线程池，池里最小3个线程，最大100，队列最大100

//房间内的食物信息

class Food {
public:
    int cx;
    int cy;
    int radius = 2;
    int color[3];
    bool is_alive = true;
    Food() {
    }
    Food(int food_cx, int food_cy, int food_r, int food_g, int food_b) {
        this->cx = food_cx;
        this->cy = food_cy;
        this->color[0] = food_r;
        this->color[1] = food_g;
        this->color[2] = food_b;
    }
    char* toString() {
        static char tmp[255];
        //FOOD cx cy radius,color r,g,b,is_alive
        sprintf(tmp, "FOOD,%d,%d,%d,%d,%d,%d,%d;", this->cx, this->cy, this->radius, this->color[0], this->color[1], this->color[2], this->is_alive);
        return tmp;
    }

};

class Ball :public Food {
public:
    char* username;
    int timestamp;
    int sx;
    int sy;
    //行为
    //移动 : 在哪里移动
    Ball( char* ball_username, int ball_cx, int ball_cy, int ball_r, int ball_g, int ball_b, int ball_radius, int ball_sx, int ball_sy) :Food(ball_cx, ball_cy, ball_r, ball_g, ball_b) {
        this->username = ball_username;
        strtok(ball_username, "$");
        int room_num = atoi(strtok(NULL, "$"));
        this->timestamp = room_num;

        this->radius = ball_radius;
        this->sx = ball_sx;
        this->sy = ball_sy;

    }
    void move() {
        this->cx += this->sx;
        this->cy += this->sy;
        //圆心点发生变化  需要做边界判断
            //横向出界
        if ((this->cx - this->radius <= 0) || (this->cx + this->radius) >= windows_width) {
            this->sx = -this->sx;
        }
        //纵向
        if ((this->cy - this->radius <= 0) || (this->cy + this->radius) >= windows_hight) {
            this->sy = -this->sy;
        }
    }
    void eat_food(Food* other) {
        //吃食物
        if (this->is_alive && other->is_alive) {
            int two_distance = pow((this->cx - other->cx), 2) + pow((this->cy - other->cy), 2);
            if ((two_distance < pow((this->radius + other->radius), 2)) && (this->radius > other->radius)) {
                other->is_alive = false;
                if (rand() % 5 == 0)
                {
                    //20%的概率加一
                    this->radius += other->radius;
                }
            }
        }
        return;
    }
    void eat_ball(Ball* other) {
        //吃球
        if (this->is_alive && other->is_alive) {
            int two_distance = pow((this->cx - other->cx), 2) + pow((this->cy - other->cy), 2);
            if ((two_distance < pow((this->radius + other->radius), 2)) && (this->radius > other->radius)) {
                other->is_alive = false;
                if (other->radius <= 7) {
                    if (rand() % 7 == 0) {
                        this->radius += 2;
                    }
                }
                else
                    this->radius += int(other->radius * 0.14);
            }
        }
        return;
    }
    bool equal(Ball* other) {
        if ((strcmp(this->username, other->username)) == 0) {
            //username相等，说明是同一个球
            return true;
        }
        return false;
    }
    char* toString() {
        static char tmp[255];
        printf("this-> username = %s\n", this->username);
        printf("this-> timestamp = %d\n", this->timestamp);
        char* tmp_username = strtok(username, "$");
        sprintf(tmp, "BALL,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d;", tmp_username, this->cx, this->cy, this->radius, this->sx, this->sy, this->color[0], this->color[1], this->color[2], this->is_alive);
        return tmp;
    }
};

vector<Food> rooms_foods[10];
vector<Ball> rooms_balls[10];

//无名信号量
sem_t rooms_sem[10];

void* user_operator(void* arg) {
    //提取socketfd
    int sockfd = *((int*)arg);

    //初始化
    char buf[BUF_SIZE], message[BUF_SIZE];
    bzero(buf, BUF_SIZE);
    bzero(message, BUF_SIZE);
    // receive message
    int len = recv(sockfd, buf, BUF_SIZE, 0);
    if (len == 0)  // len = 0 玩家退出
    {
        map<int, int>::iterator iter;
        iter = socket_room.find(sockfd);
        map<int, char*>::iterator userItem;
        userItem = socket_username.find(sockfd);
        if (iter != socket_room.end() && userItem != socket_username.end())
        {
            room[iter->second] -= 1;//房间人数减一
            int room_num = iter->second;
            char* username = userItem->second;
            sem_wait(&rooms_sem[room_num]);
            int tmp = 0;
            for (int i = 0; i < rooms_balls[room_num].size(); i++) {
                if (strcmp(rooms_balls[room_num][i].username, username) == 0) {
                    tmp = i;
                }
            }
            rooms_balls[room_num].erase(rooms_balls[room_num].begin() + tmp);

            int value;
            sem_getvalue(&rooms_sem[room_num], &value);

            if (value == 0)
            {
                sem_post(&rooms_sem[room_num]);
            }

        }




        close(sockfd);
        clients_list.remove(sockfd); //server remove the client
    }
    if (startWith(buf, "ENTERROOM")) {
        //进入房间
        strtok(buf, "_");
        int room_num = atoi(strtok(NULL, "_"));

        char* username = strtok(NULL, "_");
        printf("%d room :%s", room_num, username);

        socket_room[sockfd] = room_num;//socket 记录room
        socket_username[sockfd] = username;//socket -> username;


        sem_wait(&rooms_sem[room_num]);

        if (room[room_num] == 0) {
            room[room_num] += 1;
            for (int i = 0; i < 200; i++) {
                Food foo(rand() % windows_width, rand() % windows_hight, rand() % 256, rand() % 256, rand() % 256);
                rooms_foods[room_num].push_back(foo);
            }
        }
        else {
            room[room_num] += 1;
        }
        Ball bal(username, rand() % windows_width, rand() % windows_hight, rand() % 256, rand() % 256, rand() % 256, 5, 0, 0);
        rooms_balls[room_num].push_back(bal);
        sem_post(&rooms_sem[room_num]);
        usleep(100);
    }
    if (startWith(buf, "OPERATOR")) {
        //OPERATOR_roomnum_username_(operator:up down left right)
        strtok(buf, "_");
        int room_num = atoi(strtok(NULL, "_"));
        char* username = strtok(NULL, "_");
        char* user_operator = strtok(NULL, "_");
        int tmp = 0;

        strtok(username, "$");
        int timestamp = atoi(strtok(NULL, "$"));
        printf("user cao zuo:%s\n", user_operator);
        sem_wait(&rooms_sem[room_num]);
        for (int i = 0; i < rooms_balls[room_num].size(); i++) {
            printf("loog this time:%d\n", rooms_balls[room_num][i].timestamp);
            printf("send time :%d\n", timestamp);
            printf("between %d\n", timestamp == rooms_balls[room_num][i].timestamp);
            printf("%s == %s  res= %d\n", rooms_balls[room_num][i].username, username, strcmp(rooms_balls[room_num][i].username, username));
            /*if (strcmp(rooms_balls[room_num][i].username, username) == 0) {
                tmp = i;
            }*/
            if (timestamp == rooms_balls[room_num][i].timestamp) tmp = i;
        }
        Ball bal(rooms_balls[room_num][tmp]);
        rooms_balls[room_num].erase(rooms_balls[room_num].begin() + tmp);

        printf("new bal username%s\n", bal.username);

        if (startWith(user_operator, "UP"))
        {
            bal.sy = -2;
            bal.sx = 0;
        }
        if (startWith(user_operator, "DOWN"))
        {
            bal.sy = 2;
            bal.sx = 0;


        }
        if (startWith(user_operator, "LEFT"))
        {
            bal.sx = -2;
            bal.sy = 0;
        }
        if (startWith(user_operator, "RIGHT"))
        {
            bal.sx = 2;
            bal.sy = 0;
        }
        rooms_balls[room_num].push_back(bal);

        sem_post(&rooms_sem[room_num]);
        usleep(100);

    }
}

void* broadcast_message_in_a_room(void* argv) {
    /*广播一个房间的游戏信息*/
    int room_id;//提取room_id
    int* p = (int*)argv;
    room_id = *p;
    //初始化数据
    char message[BUF_SIZE];
    bzero(message, BUF_SIZE);
    strcat(message, "(");
    //访问临界区数据
    sem_wait(&rooms_sem[room_id]);
    //吃球、食物
    for (unsigned int i = 0; i < rooms_balls[room_id].size(); i++) {
        //遍历room_id房间内的球
        rooms_balls[room_id][i].move();//球移动
        if (rooms_balls[room_id][i].is_alive) {
            for (int b1 = 0; b1 < rooms_balls[room_id].size(); b1++) {
                //遍历球 判断能不能吃
                rooms_balls[room_id][i].eat_ball(&rooms_balls[room_id][b1]);
            }
            for (int f1 = 0; f1 < rooms_foods[room_id].size(); f1++) {
                //遍历球 判断能不能吃
                rooms_balls[room_id][i].eat_food(&rooms_foods[room_id][f1]);
            }
        }
        else {
            rooms_balls[room_id].erase(rooms_balls[room_id].begin() + i);//如果球死了，清除球
        }
    }
    for (unsigned int f = 0; f < rooms_foods[room_id].size(); f++) {
        if (!rooms_foods[room_id][f].is_alive) {
            //如果食物死了，情理 同时添加新的食物
            rooms_foods[room_id].erase(rooms_foods[room_id].begin() + f);
            Food foo(rand() % windows_width, rand() % windows_hight, rand() % 256, rand() % 256, rand() % 256);
            rooms_foods[room_id].push_back(foo);
        }
    }
    //将球的数据变为字符串
    for (unsigned int i = 0; i < rooms_balls[room_id].size(); i++)
    {
        strcat(message, rooms_balls[room_id][i].toString());
    }
    //将食物的数据变为字符串
    for (unsigned int i = 0; i < rooms_foods[room_id].size(); i++)
    {
        strcat(message, rooms_foods[room_id][i].toString());
    }

    sem_post(&rooms_sem[room_id]);
    strcat(message, ")");
    //发送数据
    for (std::map<int, int>::iterator it = socket_room.begin(); it != socket_room.end(); it++) {
        if (it->second == room_id)
        {   
            signal(SIGALRM, SIG_IGN);
            signal(SIGPIPE, SIG_IGN);
            int ret = send(it->first, message, BUF_SIZE, 0);
            if (ret < 0) {
                printf("cuowuma:%d\n", errno);
                if (errno == EAGAIN) {
                    sleep(1);//等待缓冲区释放
                    continue;
                }
                if (errno == EPIPE) {
                    continue;
                }
            }
            else {
                printf("fa song cheng gong\n");
            }
            //延迟1000weis
            usleep(100000);
        }
    }
}

void* broadcast_message(void* arg) {
    while (1) {
        //主循环

        for (int i = 0; i < 10; i++) {

            if (room[i] > 0) {


                broadcast_message_in_a_room((void*)&i);
                //延迟0.25s
                usleep(100);

            }
        }
        usleep(50000);



    }

}

int main()
{   
    pool_init(5);/*线程池中最多三个活动线程*/

    //防止管道破裂
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);
    signal(SIGALRM, SIG_IGN);
   
    //初始化信号量
    for (int i = 0; i < 10; i++) {
        sem_init(&rooms_sem[i], 0, 1);
    }

    //服务器IP + port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = PF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    //创建监听socket
    int listener = socket(PF_INET, SOCK_STREAM, 0);
    if (listener < 0) { perror("listener"); exit(-1); }
    printf("listen socket created \n");
    //绑定地址
    if (bind(listener, (struct sockaddr*) & serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind error");
        exit(-1);
    }
    //监听
    int ret = listen(listener, 5);
    if (ret < 0) { perror("listen error"); exit(-1); }
    //在内核中创建事件表
    int epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) { perror("epfd error"); exit(-1); }
    printf("epoll created, epollfd = %d\n", epfd);
    static struct epoll_event events[EPOLL_SIZE];
    //往内核事件表里添加事件
    addfd(epfd, listener, true);

    //线程池：广播信息
    pool_add_worker( broadcast_message, NULL);

    //主循环 - 处理用户数据
    while (1)
    {
        //epoll_events_count表示就绪事件的数目
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }

        printf("epoll_events_count = %d\n", epoll_events_count);
        //处理这epoll_events_count个就绪事件
        for (int i = 0; i < epoll_events_count; ++i)
        {
            int sockfd = events[i].data.fd;
            //新用户连接
            if (sockfd == listener)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrLength = sizeof(struct sockaddr_in);
                int clientfd = accept(listener, (struct sockaddr*) & client_address, &client_addrLength);

                printf("client connection from: %s : % d(IP : port), clientfd = %d \n",
                    inet_ntoa(client_address.sin_addr),
                    ntohs(client_address.sin_port),
                    clientfd);

                addfd(epfd, clientfd, true);////把这个新的客户端添加到内核事件列表

                // 服务端用list保存用户连接
                clients_list.push_back(clientfd);

                // 服务端发送房间信息信息
                printf("welcome message\n");
                char message[BUF_SIZE];
                bzero(message, BUF_SIZE);

                memcpy(message, "ROOM_", 5);
                for (int i = 0; i < 10; i++) {
                    char tmp[255];
                    sprintf(tmp, "%d,", room[i]);
                    strcat(message, tmp);
                }
                for (int ret = send(clientfd, message, BUF_SIZE, 0); ret < 0;) {
                    perror("send room message error");
                }
            }
            //客户端唤醒
            //处理用户发来的消息，并广播，使其他用户收到信息
            else
            {
                //线程池启动任务
                pool_add_worker( user_operator, &sockfd);
                usleep(10);
            }
        }
    }


    //等子线程完成任务 
    sleep(10000);
    printf("clean up thread pool\n");
    sleep(1);

    pool_destroy();

    close(listener); //关闭socket
    close(epfd);    //关闭内核   不在监控这些注册事件是否发生

    return 0;
}




