#ifndef GROUP_H
#define GROUP_H

#include<string>
#include<vector>
#include "groupuser.hpp"
using namespace std;

// allGroup 表的ORM框架
class Group
{
private:
    /* data */
    int id; // 组id
    string name; // 组name
    string desc; // 组功能描述
    vector<GroupUser> users; // 每个组的成员

public:
    Group(int id = -1,string name = "",string desc = ""):id(id),name(name),desc(desc){
        
    }

    int getId(){
        return this->id;
    }
    string getName(){
        return this->name;
    }
    string getDesc(){
        return this->desc;
    }

    vector<GroupUser> &getUsers(){
        return this->users; // 返回原对象的引用，无需拷贝
    }

    void setId(int id){
        this->id  = id;
    }

    void setName(string name){
        this->name  = name;
    }

    void setDesc(string desc){
        this->desc  = desc;
    }
};

#endif
