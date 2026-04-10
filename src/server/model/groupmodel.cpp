#include "groupmodel.hpp"
#include "db.h"

// 创建群组
bool GroupModel::createGroup(Group &group){
    // 向allgroup表中添加群组信息
    // 组装sql语句
    char sql[1024] = {0};
    // 主键会自增
    sprintf(sql,"insert into allgroup(groupname,groupdesc) values('%s','%s')",
        group.getName().c_str(),group.getDesc().c_str()); // 将string转换为char*类型指针

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
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
        
    }
    return false;
}

// 加入群组 用户加入到群组
void GroupModel::addGroup(int userid,int groupid,string role){
    // 组装sql语句
    char sql[1024] = {0};
    // 主键会自增
    sprintf(sql,"insert into groupuser(groupid,userid,grouprole) values('%d','%d','%s')",
        groupid,userid,role.c_str()); // 将string转换为char*类型指针

    MySQL mysql;
    if (mysql.connect()) // 连接数据库
    {
        /* code */
        // 更新数据库
        mysql.update(sql);
        
    }
}

// 查询用户所在群组信息
vector<Group> GroupModel::queryGroup(int userid){
    // 组装sql语句 
    char sql[1024] = {0};

    vector<Group> groupVec; // 存储用户所在的所有群组

    // 查询群组信息
    /*
    1.先根据userid在groupuser表中查询出该用户所在的群组信息
    2.再根据群组信息，查询属于该群组的所有用户的userid，并且和user表进行多表联合查询，查出用户的详细信息
    */
    // 多表联合查询可以减轻数据库频繁连接访问的压力 
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from allgroup a inner join groupuser g ON a.id = g.groupid where g.userid = %d",
        userid); // 将string转换为char*类型指针

    MySQL mysql;
    if (mysql.connect()) // 连接数据库
    {
        /* code */
        // 更新数据库
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) // 查询成功
        {
            /* code */
            MYSQL_ROW row ;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                /* code */
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }

            mysql_free_result(res);
        }
    }
    
    // 查询群组的用户信息
    for(Group &group : groupVec){
        sprintf(sql,"select u.id,u.name,u.state,g.grouprole from user u inner join groupuser g ON u.id = g.userid where g.groupid = %d ",
        group.getId()); // 将string转换为char*类型指针
        MySQL mysql;
        if (mysql.connect()) // 连接数据库
        {
            /* code */
            // 更新数据库
            MYSQL_RES *res = mysql.query(sql);
            if (res != nullptr) // 查询成功
            {
                /* code */
                MYSQL_ROW row ;
                while ((row = mysql_fetch_row(res)) != nullptr)
                {
                    /* code */
                    // 将用户信息写入到相应的groupuser中
                    GroupUser user;
                    user.setId(atoi(row[0]));
                    user.setName(row[1]);
                    user.setState(row[2]);
                    user.setRole(row[3]);
                    group.getUsers().push_back(user); // 通过引用直接修改group内部的users，防止修改的是一个临时对象
                }

                mysql_free_result(res);
            }
        }
    }
    return groupVec;
}

// 群聊业务 群聊就需要拿到所在的群的其它userid，然后之前在业务层（chatservice.hpp）中的_userConnMap成员保存了用户的连接信息，利用这个_userConnMap找到群组中的用户的连接信息再通过服务器转发，如果出现群成员离线就存储到离线消息表   
// 根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其他成员群发消息
vector<int> GroupModel::queryGroupUsers(int userid,int groupid){
    // 组装sql语句 
    char sql[1024] = {0};

    sprintf(sql,"select userid from groupuser where groupid = %d and userid != %d",
        groupid,userid); // 将string转换为char*类型指针
    
    vector<int> idVec;

    MySQL mysql;
    if (mysql.connect()) // 连接数据库
    {
        /* code */
        // 更新数据库
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) // 查询成功
        {
            /* code */
            MYSQL_ROW row ;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                /* code */
                idVec.push_back(atoi(row[0]));
            }

            mysql_free_result(res);
        }
    }
    return idVec; 
}