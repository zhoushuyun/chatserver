#include "db.h"


// 初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr); // 开辟一块存储连接资源连接数据的资源空间
}

// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}

// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
        password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr) // 连接成功 
    {
        mysql_query(_conn, "set names gbk"); // 支持中文 因为C/C++都默认ASCII码，如果直接拉取中文会出现乱码
        // 关键：设置连接字符集
        mysql_set_character_set(_conn, "utf8mb4");
        LOG_INFO<<"connect mysql success!";
    }else{ // 连接失败
        LOG_INFO<<"connect mysql fail!";
    }
    return p;
}

// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
            << sql << "更新失败!"
            <<"! MySQL error: " 
            << mysql_errno(_conn) << " - " << mysql_error(_conn);;
        return false;
    }
    return true;
}

// 查询操作 返回查询结果
MYSQL_RES* MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
            << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

// 获取连接 用来访问私有的成员变量
MYSQL* MySQL::getConnection(){
    return _conn;
}

