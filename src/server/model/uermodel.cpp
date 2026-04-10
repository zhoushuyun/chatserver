#include "usermodel.hpp"
#include "db.h"
#include<iostream>

// User表的增加方法
bool UserModel::insert(User &user){
    // 组装sql语句
    char sql[1024] = {0};
    // 主键会自增
    sprintf(sql,"insert into user(name,password,state) values('%s','%s','%s')",
        user.getName().c_str(),user.getPwd().c_str(),user.getState().c_str()); // 将string转换为char*类型指针

    MySQL mysql;
    if (mysql.connect()) // 连接数据库
    {
        /* code */
        // 更新数据库
        if (mysql.update(sql)) // 更新成功
        {
            /* code */
            // 获取插入成功的用户数据生成的主键id
            // mysql_insert_id() 是一个 MySQL C API 函数，用于获取最后一次 INSERT 操作产生的自增主键 ID
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
        
    }
    return false;
}

 // 根据用户id查询用户信息
User UserModel::query(int id){
    // 组装sql语句 
    char sql[1024] = {0};
   
    sprintf(sql,"select * from user where id = %d",id); // 将string转换为char*类型指针

    MySQL mysql;
    if (mysql.connect()) // 连接数据库
    {
        /* code */
        // 更新数据库
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) // 查询成功
        {
            /* code */
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                /* code */
                User user;
                user.setId(atoi(row[0])); // row取出的都是字符串 需要转换成整数
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);

                mysql_free_result(res); // res是一个指针 需要释放资源 防止内存泄漏
                return user;
            }
        }
    }
    return User(); // 如果没有找到 那么返回User的构造函数，默认id为-1，name和pwd均为空
}

// 更新用户的状态信息
bool UserModel::updateState(User &user){
    // 组装sql语句
    char sql[1024] = {0};
    
    sprintf(sql,"update user set state = '%s' where id = '%d'",
        user.getState().c_str(),user.getId()); // 将string转换为char*类型指针

    MySQL mysql;
    if (mysql.connect()) // 连接数据库
    {
        /* code */
        // 更新数据库
        if (mysql.update(sql)) // 更新成功
        {
            /* code */
            
            return true;
        }
        
    }
    return false;
}
 
// 重置用户的状态信息
void UserModel::resetState(){
    // 组装sql语句
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    MySQL mysql;
    if (mysql.connect()) // 连接数据库
    {
        /* code */
        // 更新数据库
        mysql.update(sql); // 更新成功
        
    }
    
}