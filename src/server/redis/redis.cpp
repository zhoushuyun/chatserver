#include "redis.hpp"
#include <iostream>
#include <thread>
using namespace std;

Redis::Redis():_publish_context(nullptr),_subscribe_context(nullptr){

}

Redis::~Redis(){
    if (_publish_context != nullptr)
    {
        /* code */
        redisFree(_publish_context);
    }
    
    if (_subscribe_context != nullptr)
    {
        /* code */
        redisFree(_subscribe_context);
    }
}

bool Redis::connect(){
    // 负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1",6379);
    if (_publish_context == nullptr)
    {
        /* code */
        cerr<<"connect redis failed!"<<endl;
        return false;
    }
    
    // 负责subscribe发布消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1",6379);
    if (_subscribe_context == nullptr)
    {
        /* code */
        cerr<<"connect redis failed!"<<endl;
        return false;
    }

    // 在单独的线程中监听通道上的事件，有消息给业务层进行上报 todo 
    // chatservice本身就是单例模式 只需要启动一个线程
    thread t([&](){
        observer_channel_message();
    });
    t.detach();

    cout<<"connect redis-server success!"<<endl;
    return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel,string message){
    // redisCommand先将要发送的命令缓存到本地
    redisReply *reply = (redisReply *)redisCommand(_publish_context,"PUBLISH %d %s",channel,message.c_str()); // 阻塞式调用：发送命令后立即等待 Redis 返回结果 publish不会阻塞当前线程，而是立即回复，使用redisCommand没有影响
    if (reply == nullptr)
    {
        /* code */
        cerr<<"publish command fail"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
// 先使用redisAppendCommand组装好命令写入缓存，再调用redisBufferwrite从本地的缓存发送到redis server上
bool Redis::subscribe(int channel){
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    
    
    /*
    redisAppendCommand 是 hiredis 库中的一个异步 API，它允许你将 Redis 命令追加到输出缓冲区中，而不立即等待响应。这对于实现非阻塞操作特别有用
    原理：redisAppendCommand将数据发送到缓冲区中，redisBufferWrite再把数据发生给redis服务器中
    */
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context,"SUBSCRIBE %d",channel)) // 将命令缓存到本地 非阻塞缓存‌：仅将命令放入客户端输出缓冲区（output buffer），‌不立即发送‌，也‌不等待响应‌。
    {
        /* code */
        cerr<<"subscibe command failed!"<<endl;
        return false;
    }
    
    // redisBufferwrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        /* code */
        // 正常情况下，它返回 REDIS_OK (值为0)；如果发生错误，则返回 REDIS_ERR (值为-1)。
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context,&done)){
            cerr<<"subscibe command failed!"<<endl;
            return false;
        }
    }
    // redisReply 以阻塞方式等待远端响应 
    return true;
    
}

// 向redis指定的通道unsubscribe取消订阅消息 用户下线
bool Redis::unsubscribe(int channel){
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context,"UNSUBSCRIBE %d",channel)) // 将命令缓存到本地 非阻塞缓存‌：仅将命令放入客户端输出缓冲区（output buffer），‌不立即发送‌，也‌不等待响应‌。
    {
        /* code */
        cerr<<"unsubscibe command failed!"<<endl;
        return false;
    }
    
    // redisBufferwrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while (!done)
    {
        /* code */
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context,&done)){
            cerr<<"unsubscibe command failed!"<<endl;
            return false;
        }
    }
    // redisReply 以阻塞方式等待远端响应 
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message(){
    redisReply *reply = nullptr;
    // subscribe` 需要异步处理，异步必须和事件循环绑定
    while (REDIS_OK == redisGetReply(this->_subscribe_context,(void **)&reply)) // redisGetReply负责从 Redis 服务器接收回复。
    {
        /* code */
        // 订阅命令的回复一定是数组element
        // 元素1: 消息类型（"subscribe", "unsubscribe", "message"）
        // 元素2: 通道号
        // 元素3: 订阅数 或 消息内容
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            /* code */
            // 给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr<< "------- observer_channel_message quit -----------" <<endl;
}

// 初始向业务层上报通道消息的回调对象 对外公有方法
void Redis::init_notify_handler(function<void(int,string)> fn){
    this->_notify_message_handler = fn;
}