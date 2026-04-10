#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client的公共文件
*/
enum EnMsgType{
    LOGIN_MSG = 1, // 登录消息
    LOGIN_MSG_ACK, // 登录响应消息 在登录成功后服务器返回一个ACK
    LOGINOUT_MSG, // 注销消息
    REG_MSG,   // 注册消息 自动+1赋值 客户端发送
    REG_MSG_ACK, // 注册响应消息 在注册成功后服务器返回一个ACK
    ONE_CHAT_MSG, // 聊天消息
    ADD_FRIEND_MSG, //添加好友消息

    CREATE_GROUP_MSG, // 创建群组
    ADD_GROUP_MSG, //加入群组
    GROUP_CHAT_MSG, // 群聊天
};

#endif