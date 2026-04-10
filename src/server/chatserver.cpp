#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
#include <functional> // 函数对象绑定器
#include <string>
using namespace std;
using namespace placeholders;
using namespace nlohmann;
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg) : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册连接的回调函数
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));

    // 注册消息的回调函数 网络层到业务层的桥梁
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报连接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端端开连接
    if (!conn->connected())
    {
        /* code */
        Chatservice::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化 相当于将json字符串解码成json对象
    json js = json::parse(buf);
    // 达到的目的：完全解耦网络模块的代码和业务模块的代码
    // 通过js['msgid']获取业务处理器handler->conn、js、time
    auto msgHandler = Chatservice::instance()->getHandler(js["msgid"].get<int>()); // 获取唯一实例  get<int>():获取整数
    // 回调消息绑定好的事件处理器，来执行相应的业务处理 转发
    msgHandler(conn, js, time);
}