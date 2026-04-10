#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

// 群组用户 多了一个role权限信息，其他信息从User类中继承，复用User的其它消息
class GroupUser:public User
{
private:
    /* data */
    string role; // 权限信息

public:
    void setRole(string role){
        this->role = role;
    }
    
    string getRole(){
        return this->role;
    }
};

#endif