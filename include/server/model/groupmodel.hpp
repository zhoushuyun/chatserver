#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"

// 维护群组信息的操作接口方法
class GroupModel
{
private:
    /* data */
public:
    // 创建群组
    bool createGroup(Group &group);

    // 加入群组 用户加入到群组
    void addGroup(int userid,int groupid,string role);

    // 查询用户所在群组信息
    vector<Group> queryGroup(int userid);

    // 群聊业务 群聊就需要拿到所在的群的其它userid，然后之前在业务层（chatservice.hpp）中的_userConnMap成员保存了用户的连接信息，利用这个_userConnMap找到群组中的用户的连接信息再通过服务器转发，如果出现群成员离线就存储到离线消息表   
    // 根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其他成员群发消息
    vector<int> queryGroupUsers(int userid,int groupid);
};

#endif