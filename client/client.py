import numpy
import pygame
import random
import time
import _thread
import socket
username = ""

import threading


class Mythread(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
    # 重写线程代码
    def run(self):
        # #接受游戏信息
        game_data=""
        tmp_game_data = ""
        while True:
            #主循环
            recv_game_message_none_flag = True #没有收到任何消息
            
            # while recv_game_message_none_flag:
            game_data = client.recv(1024).decode().strip(b'\x00'.decode())
            if game_data:
              
                tmp_game_data += game_data
                if tmp_game_data[0] == "(" and tmp_game_data[-1]==")":
                    tmp_game_data = tmp_game_data.split('(')[-1]
             
                    global balls
                    global foods
                    lock.acquire()
                    balls = []
                    foods = []

                    for i in tmp_game_data[0:-1].split(';')[:-1]:
                        attr = i.split(',')

                        if attr[0] == "BALL":
                            bal = Ball(attr[1],int(attr[2]),int(attr[3]),int(attr[4]),int(attr[5]),int(attr[6]),(int(attr[7]),int(attr[8]),int(attr[9])),int(attr[10]))
                            balls.append(bal)
                        if attr[0] == "FOOD":  
                            foo = Food(int(attr[1]),int(attr[2]),int(attr[3]),(int(attr[4]),int(attr[5]),int(attr[6])),int(attr[7]))
                            foods.append(foo)
                        else:
                            print(attr[0])   
  
                    lock.release()
                    time.sleep(0.01)
                    tmp_game_data = ""#重置
            else:
                pass
            #     print("没有收到数据")
            # print("#################################")
            time.sleep(0.01)


           

            

class Color:
    
    @classmethod
    def random_color(cls):
        red = random.randint(0, 255)
        green = random.randint(0, 255)
        blue = random.randint(0, 255)
        return (red, green, blue)
     # 出现对应 在界面上画出对应的一个球
    
class Food:
    def __init__(self,cx,cy,radius,color,is_alive):
        # 食物的位置
        self.cx = cx
        self.cy = cy
        self.radius = radius
        self.color = color
        self.is_alive = is_alive  # 球的默认存活状态
    def draw(self, window):
        pygame.draw.circle(window, self.color, (self.cx, self.cy), self.radius)
        
class Ball:
    def __init__(self, username,cx, cy, radius, sx, sy, color,is_alive):
        self.username = username
        # 球的位置
        self.cx = cx
        self.cy = cy
        # 球的半径
        self.radius = radius
        # 球的速度
        self.sx = sx
        self.sy = sy
        self.color = color
        self.is_alive = is_alive  # 球的默认存活状态

    def drawText(self,screen,text,posx,posy):
        fontColor=Color.random_color()
        
        fontObj = pygame.font.SysFont("宋体",self.radius//3) # 通过字体文件获得字体对象
        textSurfaceObj = fontObj.render(text, True,fontColor,self.color)  # 配置要显示的文字
        textRectObj = textSurfaceObj.get_rect()  # 获得要显示的对象的rect
        textRectObj.center = (posx, posy)  # 设置显示对象的坐标
        screen.blit(textSurfaceObj, textRectObj)  # 绘制字
    # 出现对应 在界面上画出对应的一个球

    def draw(self, window):
        pygame.draw.circle(window, self.color, (self.cx, self.cy), self.radius)
        self.drawText(window,self.username,self.cx,self.cy)

if __name__ == '__main__':
    print("""
     ____ ___ ____   ____    _    _     _     
    | __ )_ _/ ___| | __ )  / \  | |   | |    
    |  _ \| | |  _  |  _ \ / _ \ | |   | |    
    | |_) | | |_| | | |_) / ___ \| |___| |___ 
    |____/___\____| |____/_/   \_\_____|_____|
     游戏：大球吃小球
     作者：贾敬哲
     学号：20175276                             
    """)
    #连接服务器
    HostPort = ("192.144.145.17",8888)
    client= socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    client.connect(HostPort)
    
    #接受房间信息
    recv_none_flag = True #没有收到任何消息
    while recv_none_flag:
        time.sleep(0.1)
        #每0.5秒检查下
        room_message = client.recv(1024).decode().strip(b'\x00'.decode())
        if room_message:
            #收到信息了
            recv_none_flag = False
    #房间信息
    room = [int(i) for i in room_message[5:].split(",")[:-1]]
    print("\t房间信息:\n")
    for i in range(0,len(room),2):
        if i+1 >= len(room):
            print("\t\t房间%d: %d/ 10\t"%(i,room[i]))
        else:
            print("\t\t房间%d: %d/ 10\t房间%d: %d/ 10"%(i,room[i],i+1,room[i+1]))
    select_room = eval(input("请输入要选择的房间(数字)>>>"))
    while( room[select_room] >=10 ):
        print("房间已满")
        select_room =eval(input("请重新输入要选择的房间(数字)>>>"))
    #用户名
    username = input("请输入用户名>>>")
    username = username+"$"+str(int(time.time())) #加秒级时间戳做唯一表示
    time.sleep(0.1)#停止100毫秒
    #发送进入房间请求
    enter_room_message = "ENTERROOM_"+str(select_room)+"_"+username;
    client.send(enter_room_message.encode('utf-8'))

   
    balls = []
    foods = []
    lock = threading.RLock()#创建锁
    #多线程 -1.渲染 -2.接受数据
    t1 = Mythread()
    t1.start()


    # 画质屏幕
    pygame.init()
    # 设置屏幕
    screen = pygame.display.set_mode((800, 700))
    # 设置屏幕标题
    pygame.display.set_caption("大球吃小球")
    # 定义一容器  存放所有的球
   
    isrunning = True


    while isrunning:
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                isrunning = False
        
        keys = pygame.key.get_pressed()
        operator = ""
        # 方向箭头响应
        if keys[pygame.K_LEFT]:
            print("left")

            operator = "LEFT"
            #操作信息
            operator_message = "OPERATOR_"+str(select_room)+"_"+username+"_"+operator;
            print(operator_message)

            client.send(operator_message.encode('utf-8'))
        if keys[pygame.K_RIGHT]:
            print("right")

            operator = "RIGHT"
            #操作信息
            operator_message = "OPERATOR_"+str(select_room)+"_"+username+"_"+operator;
            
            client.send(operator_message.encode('utf-8'))
        if keys[pygame.K_UP]:
            print("up")

            operator = "UP"
            #操作信息
            operator_message = "OPERATOR_"+str(select_room)+"_"+username+"_"+operator;

            client.send(operator_message.encode('utf-8'))
        if keys[pygame.K_DOWN]:
            print("down")

            operator = "DOWN"
            #操作信息
            operator_message = "OPERATOR_"+str(select_room)+"_"+username+"_"+operator;

            client.send(operator_message.encode('utf-8'))
        
        # 刷漆
        screen.fill((255, 255, 255))
        # 遍历容器
        lock.acquire()
        for b in balls:
            if b.is_alive:
                print(b.username)
                b.draw(screen)
        print(balls)
        for f in foods:
            if f.is_alive:
                f.draw(screen)
        # 渲染
        lock.release()

        pygame.display.flip()
        # 设置动画的时间延迟
        pygame.time.delay(40)


