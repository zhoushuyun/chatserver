#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include"user.hpp"
#include<vector>
using namespace std;

// 维护好友信息的操作接口方法
class FriendModel
{
private:
    /* data */
public:
    // 添加好友关系
    void insert(int userid,int friendid);

    // 返回用户好友列表 friendid user表 -> friendname、state 多表联合查询
    vector<User> query(int userid);
};

#endif 
