#include "chatservice.hpp"
#include "public.hpp"
#include "muduo/base/Logging.h" // muduo库日志
#include<vector>
using namespace std;
using namespace muduo;

Chatservice * Chatservice::instance(){ // 静态方法在类外实现的时候不需要再声明static关键字
    static Chatservice service; // C++11线程安全
    return &service;
}

// 注册消息以及对应的Handler回调操作
Chatservice::Chatservice(){
    // 业务模块的核心：将网络模块和业务模块进行解耦
    // 添加新的业务就要添加新的消息id和事件回调

    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&Chatservice::login,this,_1,_2,_3)}); // 3个参数 对应3个占位符
    _msgHandlerMap.insert({REG_MSG,std::bind(&Chatservice::reg,this,_1,_2,_3)}); // 3个参数 对应3个占位符
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&Chatservice::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&Chatservice::addFriend,this,_1,_2,_3)});
    
    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&Chatservice::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&Chatservice::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&Chatservice::groupChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG,std::bind(&Chatservice::loginout,this,_1,_2,_3)});

    // 连接redis服务器
    if (_redis.connect())
    {
        /* code */
        // 设置上报信息的回调 
        _redis.init_notify_handler(std::bind(&Chatservice::handleRedisSubscribeMessage,this,_1,_2)) ;
    }
    
}

// 获取消息对应的处理器
MsgHandler Chatservice::getHandler(int msgid){
    // 记录错误日志 msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end()) // 不存在 返回一个默认的处理器，空操作
    {
        /* code */
        // 使用muduo库的日志打印 更详细
        // 错误消息
        // LOG_ERROR<<"msgid:"<<msgid<<" can not find handler!"; // muduo库会自动输出endl
        return [=](const TcpConnectionPtr &conn,json &js,Timestamp time){ // [=]：按值捕获（复制）外部作用域中所有可见的变量
            LOG_ERROR<<"msgid:"<<msgid<<" can not find handler!";
        };

    }else{
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务 id pwd
// ORM框架 （Object Relational Mapping，对象关系映射）
// 在业务层操作的都是对象，看不到具体的SQL语句；而在数据层（DAO）才真正有数据库的操作
void Chatservice::login(const TcpConnectionPtr&conn,json &js,Timestamp time){
    int id = js["id"].get<int>(); // 将底层数据转换成整型
    string pwd = js["password"];

    User user = _userModel.query(id); // 查询id值，返回主键id对应的数据
    if (user.getId() == id && user.getPwd() == pwd) // 登录成功
    {
        if (user.getState() == "online") // 该用户已经登录，不允许重复登录
        {
            /* code */
            json response;
            response["msgid"] = LOGIN_MSG_ACK; 
            response["errno"] = 2; // 不同的错误，不同的errno
            // response["errmsg"] = "该账号已经登录，请重新输入新账号";
            response["errmsg"] = "this account is using,input another!";
            conn->send(response.dump());
        }else{

            // 临界区代码段
            {
                lock_guard<mutex> lock(_connMutex); // 构造互斥锁
                // 登录成功，记录用户连接信息 
                _userConnMap.insert({id,conn}); // 在多线程的环境下使用 注意线程安全问题
            } // 出了作用域解锁
            
            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(user.getId()); // 登录成功后订阅通道

            // 数据库的增删改查上的并发操作由mysqlServer保证，不需要考虑线程安全问题
            // 登陆成功，更新用户状态信息 offline->online
            user.setState("online");
            _userModel.updateState(user);

            // 直接保留在本地
            json response;
            response["msgid"] = LOGIN_MSG_ACK; 
            response["errno"] = 0; // 业务成功
            response["id"] = user.getId(); // 用户id
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(user.getId());
            if(!vec.empty()){ // 离线消息不为空
                response["offlinemsg"] = vec; // json可以直接序列化容器
                // 读取该用户的离线消息后，把该用户的所有离线消息删除
                _offlineMsgModel.remove(user.getId());
            }

            // 查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(user.getId());
            if(!userVec.empty()){ // 离线消息不为空
                // response["friends"] = userVec; // json不能存放自定义类型的容器
                vector<string> vec2;
                // 把userVec拿到的用户信息转成合适的json字符串，再添加到vec2中
                for (User &user:userVec)
                {
                    /* code */
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组消息
            vector<Group> groupVec = _groupModel.queryGroup(user.getId());
            if(!groupVec.empty()){ 
                // 把groupVec拿到的用户信息转成合适的json字符串，再添加到groupV中
                vector<string> groupV;
                for (Group &group:groupVec)
                {
                    /* code */
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for(GroupUser &user:group.getUsers()){
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }

            conn->send(response.dump()); // 网络模块发送给客户端的响应，告诉客户端业务处理的结果（成功/失败）。
        }
        
        /* code */
        
    }else{ // 用户不存在 登录失败 或者用户存在但是密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        if(user.getId() == -1){ // 用户不存在
            response["errno"] = 3; // 业务失败 不需要id
            // response["errmsg"] = "用户名不存在";
            response["errmsg"] = "id does not exist";
        } 
        response["errno"] = 1; // 业务失败 不需要id
        // response["errmsg"] = "用户名或者密码错误";
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump()); // 网络模块发送给客户端的响应，告诉客户端业务处理的结果（成功/失败）。
    }
}

// 处理注册业务 name password
void Chatservice::reg(const TcpConnectionPtr&conn,json &js,Timestamp time){
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state) // 插入成功
    {
        /* code */
        json response;
        response["msgid"] = REG_MSG_ACK; 
        response["errno"] = 0; // 业务成功
        response["id"] = user.getId(); // 用户id
        conn->send(response.dump()); // 网络模块发送给客户端的响应，告诉客户端业务处理的结果（成功/失败）。
    }else{ // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK; 
        response["errno"] = 1; // 业务失败 不需要id
        conn->send(response.dump()); // 网络模块发送给客户端的响应，告诉客户端业务处理的结果（成功/失败）。
    }
    
}

// 处理注销业务
void Chatservice::loginout(const TcpConnectionPtr&conn,json &js,Timestamp time){
    int userid = js["id"];
    
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            /* code */
            _userConnMap.erase(it);

        }
    }

    // 用户注销，相当于下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    // 更新用户状态信息
    User user(userid,"","","offline"); // 只需要userid和offline
    _userModel.updateState(user);

}

// 处理客户端异常退出 异常退出没有id，网络层出现问题只能拿到发生异常的连接
void Chatservice::clientCloseException(const TcpConnectionPtr &conn){
    User user;
    {
        lock_guard<mutex> lock(_connMutex); // 加锁
        // 查找    
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            /* code */
            if (it->second == conn) 
            {
                /* code */
                // 从map表删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());
    
    // 更新用户的状态信息
    if (user.getId() != -1) // 如果遍历map表没有找到对应的id，那么user的成员id默认值就为-1 不再向数据库进行更新操作
    {
        /* code */
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 一对一聊天业务 由于是由网络层派发的处理器回调，参数一致
void Chatservice::oneChat(const TcpConnectionPtr&conn,json &js,Timestamp time){
    // 获取to字段
    int toid = js["toid"].get<int>();

    // 在_userConnMap中查找这个id 以查看是否在线
    {
        // 访问连接信息表 要保证线程安全
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        // 不能在锁外处理，否则如果用户在线在锁外处理 没有对这个连接信息表加锁，此时如果该连接被删除就无法保证线程安全
        if (it != _userConnMap.end())  // toid在线，转发消息 在同一台服务器
        {
            /* code */
            // 服务器主动推送消息给toid用户
            it->second->send(js.dump()); // 发送给it->second（目标用户的连接）
            return;
        }
    }

    // 查阅toid是否在线
    User user = _userModel.query(toid);
    if (user.getState() == "online") // 在线但不在同一台服务器->发布到消息队列
    {
        /* code */
        _redis.publish(toid,js.dump());
        return;
    }
    
    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid,js.dump());
}

// 服务器异常，业务重置方法
void Chatservice::reset(){
    // 把online状态的用户，设置成offline
    _userModel.resetState();

}

// 添加好友业务 msgid id friendid
void Chatservice::addFriend(const TcpConnectionPtr&conn,json &js,Timestamp time){
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid,friendid);
    _friendModel.insert(friendid,userid); // 添加反向关系
} 

// 创建群组业务 creator
void Chatservice::createGroup(const TcpConnectionPtr&conn,json &js,Timestamp time){
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的新群组信息
    Group group(-1,name,desc);
    if (_groupModel.createGroup(group)) // 建立群组和用户之间的关系
    {
        /* code */
        // 存储群组创建人信息 createGroup函数只是写入了群的信息，没有人的关联关系，通过addGroup关联起来
        _groupModel.addGroup(userid,group.getId(),"creator");
    }
}

// 加入群组业务 normal
void Chatservice::addGroup(const TcpConnectionPtr&conn,json &js,Timestamp time){
    int userid  = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid,groupid,"normal");
}

// 群聊业务
void Chatservice::groupChat(const TcpConnectionPtr&conn,json &js,Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid,groupid); // 查询群组中其他用户的id

    lock_guard<mutex> lock(_connMutex); // 如果把锁放在循环内部会不断进行加锁解锁的操作
    for(int id:useridVec){
        
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end()) // 用户在线且位于同一个服务器
        {
            /* code */
            it->second->send(js.dump());
        }else{ // 不在线 可能位于不同的服务器
            // 查询toid是否在线
            User user = _userModel.query(id);
            if (user.getState() == "online") // 用户在线且位于不同服务器
            {
                /* code */
                _redis.publish(id,js.dump());
            }else{ // 用户不在线 存储到离线消息表
                _offlineMsgModel.insert(id,js.dump());
            }
        }
    }
}   

// 从redis消息队列中获取订阅的消息

/*
    这个回调函数是 Redis 订阅者收到消息时的处理函数，参数应该是：
    channel：消息来自哪个通道（比如 "user_123" 这样的用户专属通道）
    message：通道上发布的消息内容
*/
void Chatservice::handleRedisSubscribeMessage(int userid,string msg) // redis上报给业务层的参数为通道号和通道发生的消息
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end()){
        it->second->send(msg);
        return;
    }

    // 存储该用户离线消息
    _offlineMsgModel.insert(userid,msg); // 在向业务层上报过程中用户下线
}