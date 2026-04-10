#include "friendmodel.hpp"
#include "db.h"

// 添加好友关系
void FriendModel::insert(int userid,int friendid){
    // 组装sql语句
    char sql[1024] = {0};
    // 插入所有字段
    sprintf(sql,"insert into friend(userid,friendid) values(%d,'%d')",
        userid,friendid); // 将string转换为char*类型指针

    MySQL mysql;
    if (mysql.connect()) // 连接数据库
    {
        /* code */
        // 更新数据库
        mysql.update(sql);// 更新成功
    }
}

// 返回用户好友列表 friendid user表 -> friendname、state 多表联合查询
vector<User> FriendModel::query(int userid){
    // 组装sql语句 
    char sql[1024] = {0};
   
    sprintf(sql,"select u.id,u.name,u.state from user u inner join friend f ON f.friendid = u.id where f.userid = %d",
        userid); // 将string转换为char*类型指针

    vector<User> vec; // 记录离线消息

    MySQL mysql;
    if (mysql.connect()) // 连接数据库
    {
        /* code */ 
        // 更新数据库
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) // 查询成功
        {
            /* code */
            // 把userid用户的所有离线消息放入vector中返回
            MYSQL_ROW row;
            // mysql_fetch_row：每次读取一行数据，传入结果集MYSQL_RES结构体指针、返回MYSQL_ROW结构体row，可以通过row[0]、row[1]…下标索引的方式获取每个字段的数据（所有数据都是字符串）
            while ((row  = mysql_fetch_row(res)) != nullptr)
            {
                /* code */
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res); // 释放mysql资源
            return vec;
        }
    }
    return vec;
}