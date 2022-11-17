# ChatRoom in C++
```
A little chatroom with cpp
```
`GNU/Linux` 环境下使用 `C/S` 模型实现的简易聊天室。

### composition
- C++
- Socket
  - epoll ET
- Multi-threading
- Redis
- Makefile
- Json

# Getting started
`terminal 0`:run Redis 
```bash
redis-server
```

`terminal 1`:run server
```bash
git clone https://github.com/Daz-3ux/Chat_In_Cpp.git & cd Chat_In_Cpp

cd server && make -j8
```

`terminal 2`:run client
```bash
cd Chat_In_Cpp && cd client
make -j8
```

`run`：
```
./server 127.0.0.1 7890

//-----------------------------

./client 127.0.0.1 7890
```

# Features
- 账号管理
```
实现登录、注册、注销
实现找回密码
```
- 好友管理
```
实现好友的添加、删除、查询操作
实现显示好友在线状态
禁止不存在好友关系的用户间的私聊
实现屏蔽好友消息
实现好友间聊天
```
- 群管理
```
实现群组的创建、解散
实现用户申请加入群组
实现用户查看已加入的群组
实现群组成员退出已加入的群组
实现群组成员查看群组成员列表
实现群主对群组管理员的添加和删除
实现群组管理员批准用户加入群组
实现群组管理员/群主从群组中移除用户
实现群组内聊天功能
```
- 聊天功能（包括私聊及群聊）
```
实现消息的实时通知
实现查看历史消息记录
实现用户间在线聊天
实现在线用户对离线用户发送消息，离线用户上线后获得通知
实现在线发送文件
```
- `未能实现的功能`
```
引入线程池
断点续传
通过 邮箱/手机号 找回密码
对数据进行加密
```

# Server
- 采用 `epoll ET` 以及`多线程模型`实现
- 连接 `Redis` 数据库
- 存储 `Json` 格式的数据，数据存储于服务器本地
- 屏蔽了部分信号，提高了服务器的稳定性

# Client
- 可处理大部分用户异常输入，具有较高稳定性
- 使用Json格式化数据并传输

### such as...
- `redis`
![](https://raw.githubusercontent.com/Daz-3ux-Img/Img-hosting/master/host/202211151837787.png)

- `server`
![](https://raw.githubusercontent.com/Daz-3ux-Img/Img-hosting/master/host/202211151835260.jpg)

- `client`
![](https://raw.githubusercontent.com/Daz-3ux-Img/Img-hosting/master/host/202211151839289.png)
