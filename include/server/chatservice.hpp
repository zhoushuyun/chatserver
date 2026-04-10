#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include<unordered_map>
#include<functional>
#include<muduo/net/TcpConnection.h>
#include<mutex>
#include"json.hpp"
#include"usermodel.hpp"
#include"offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include "redis.hpp"
using json = nlohmann::json;
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn,
    json &js,Timestamp time)>; // c++11新特性：给已经存在的类型定义新的类型名称

// 聊天服务器业务类
class Chatservice
{
private:  
    Chatservice(/* args */);
    // 给msgid映射一个事件回调 一个消息id映射一个事件处理->map容器
    /* data */
    // 存储消息id和其对应的业务处理方法 运行的过程中只有读操作没有写操作（只在构造函数中运行一次） 不会出现线程安全的问题
    unordered_map<int,MsgHandler> _msgHandlerMap; // 消息id对应的处理操作
    
    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

    // 存储在线用户的通信连接 随着用户的上线和下线不断改变 出现线程安全的问题
    unordered_map <int,TcpConnectionPtr> _userConnMap;

    // 定义互斥锁 保证_userConnMap的线程安全
    mutex _connMutex;

    // redis操作对象
    Redis _redis;

public:
    // 获取单例对象的接口函数
    static Chatservice * instance(); // 构造函数是私有的，此时必须通过静态方法instance()获取唯一实例
    // 处理登录业务 网络层派发的业务都需要这三个参数
    void login(const TcpConnectionPtr&conn,json &js,Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr&conn,json &js,Timestamp time);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    // 一对一聊天业务 由于是由网络层派发的处理器回调，参数一致
    void oneChat(const TcpConnectionPtr&conn,json &js,Timestamp time);
    // 服务器异常，业务重置方法
    void reset();
    // 添加好友业务
    void addFriend(const TcpConnectionPtr&conn,json &js,Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr&conn,json &js,Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr&conn,json &js,Timestamp time);
    // 群聊业务
    void groupChat(const TcpConnectionPtr&conn,json &js,Timestamp time);
    // 处理注销业务
    void loginout(const TcpConnectionPtr&conn,json &js,Timestamp time);
    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int,string); // redis上报给业务层的参数为通道号和通道发生的消息
};
#endif