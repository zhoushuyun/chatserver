#include "json.hpp"
#include "user.hpp"
#include "group.hpp"
#include "groupuser.hpp"
#include "public.hpp"
#include<vector>
#include<iostream>
#include<thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include<unistd.h>
#include<semaphore.h>
#include<atomic>

using namespace std;
using json = nlohmann::json;

// 记录当前系统登录的用户信息 
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
// 显示当前登录成功用户的基本信息
void showCurrentUserData();
// 控制主菜单页面程序
bool isMainMenuRunning = false;

// 用于读写线程之间的通信
sem_t rwsem;
// 记录登录状态是否成功
atomic_bool g_isLoginSuccess{false};

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd); // 聊天也涉及数据的发送，因此需要clientfd

// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc,char **argv){
    if(argc < 3){ // 判断参数列表个数
        cout<<"command invalid! example:./ChatClient 127.0.0.1 8000"<<endl;
        exit(-1);
    }

    // 解析命令行参数传递的ip地址和端口
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]); // TCP/UDP 端口号的范围是 0-65535（2^16 - 1）
      
    // 创建客户端的socket
    int client_fd = socket(AF_INET,SOCK_STREAM,0);
    if (-1 == client_fd)
    {
        /* code */
        cerr<<"socket creat error"<<endl;
        exit(-1);
    }
    
    // 填写服务器的ip和端口号
    struct sockaddr_in server;
    memset(&server,0,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip); // 将网络地址字符串转换成网络所使用的二进制数字
    

    // 将客户端和服务器进行连接
    if(connect(client_fd,(struct sockaddr *)&server,sizeof(server)) == -1){
        cerr<<"connect server error"<<endl;
        close(client_fd);
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem,0,0);

    // 连接服务器成功，启动接收子线程
    thread readTask(readTaskHandler,client_fd); // c++11提供的线程库 底层对pthread库进行封装 并且支持跨平台 pthread_create
    // detach()的作用是将子线程和主线程的关联分离，也就是说detach()后子线程在后台独立继续运行，主线程无法再取得子线程的控制权，即使主线程结束，子线程未执行也不会结束。
    readTask.detach();  // 让主线程专注于连接管理和消息分发，而每个接收线程专注于各自客户端的消息接收，实现了高效的并发处理 pthread_detach

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        /* code */
        // 显示首页面菜单 登录、注册、退出
        cout<<"====================="<<endl;
        cout<<"1.login"<<endl;
        cout<<"2.register"<<endl;
        cout<<"3.quit"<<endl;
        cout<<"====================="<<endl;
        cout<<"choice:";
        int choice = 0;
        cin>>choice;
        cin.get(); // 读取缓冲区残留的回车 防止后面读取字符串的时候把残留的回车也读入
        switch (choice)
        { 
        case 1: // login业务
            {
                int id = 0;
                char pwd[50] = {0};

                cout<<"usrid:";
                cin>>id;
                cin.get();
                cout<<"password:";
                cin.getline(pwd,50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();

                g_isLoginSuccess = false; // 避免登出的时候没有置为false

                int len = send(client_fd,request.c_str(),strlen(request.c_str()) + 1,0);
                if (len == -1) // 发送失败
                {
                    /* code */
                    cerr<<"send login response msg error"<<request<<endl;
                } // 发送成功 
                    // char buffer[1024] = {0};
                    // len = recv(client_fd,buffer,1024,0);

                    // if (len == -1)
                    // {
                    //     /* code */
                    //     cerr<<"recv login response error"<<endl;
                    // }
                    // 接收成功
                    
                    
                        

                    // 登录成功，启动接收线程负责接收数据
                    // 该线程只启动一次
                    // static int readthreadnumber = 0;
                    // if (readthreadnumber  == 0 ) // 没有开启读线程
                    // {
                    //     /* code */

                    //     /*
                    //     thread readTask(readTaskHandler,client_fd); // c++11提供的线程库 底层对pthread库进行封装 并且支持跨平台 pthread_create
                    //     // detach()的作用是将子线程和主线程的关联分离，也就是说detach()后子线程在后台独立继续运行，主线程无法再取得子线程的控制权，即使主线程结束，子线程未执行也不会结束。
                    //     readTask.detach();  // 让主线程专注于连接管理和消息分发，而每个接收线程专注于各自客户端的消息接收，实现了高效的并发处理 pthread_detach
                    //     */
                        
                    //     readthreadnumber ++;
                    // }
                    
                sem_wait(&rwsem); // 等待信号量，由子线程处理完登录的响应消息后通知通知
                    
                if (g_isLoginSuccess) // 登录成功
                {
                    /* code */
                    // 进入聊天主菜单界面
                    isMainMenuRunning = true;
                    mainMenu(client_fd);
                }

            }
            
            /* code */
            
            

            break;
        
        case 2: // register业务
            {
                /* code  */
                char name[50] = {0};
                char password[50] = {0};
                cout<<"usrname:";
                cin.getline(name,50); // 如果直接使用cin>>遇到非法字符（空格、回车）会结束输入，例如当输入zhang san就只能读取一个zhang
                cout<<"password:";
                cin.getline(password,50);

                // 组装json数据
                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = password;
                string request = js.dump(); // 将json 数据对象序列化成字符串
                
                // 将字符串发送出去
                int len = send(client_fd,request.c_str(),strlen(request.c_str()) + 1,0); // +1：包含结束符\0
                if (len == -1) 
                {
                    /* code */
                    cerr<<"send reg response msg error:"<<request<<endl;
                }
                // else{
                //     char buffer[1024] = {0};
                //     len = recv(client_fd,buffer,1024,0); // 阻塞等待注册请求的响应
                //     if(len == -1){
                //         cerr<<"send reg response error"<<endl;
                //     }else{
                        
                //     }
                    
                // }
                sem_wait(&rwsem); // 等待信号量，子线程处理完注册消息会通知

                
            }
            break;

        case 3: // quit业务
            /* code */
            close(client_fd);
            sem_destroy(&rwsem); // 销毁信号量
            exit(0);
            break;
        
        default:
            cerr<<"invalid cinput!"<<endl;
            break;
        
        }
    }
    

    return 0;
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData(){
    cout<<"=======login======="<<endl;
    cout<<"current login user => id: "<<g_currentUser.getId()<<" name:"<<g_currentUser.getName()<<endl;
    cout<<"=======friend list======="<<endl;
    if(!g_currentUserFriendList.empty()){
        for(User &user : g_currentUserFriendList){
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }
    cout<<"=======group list======="<<endl;
    if(!g_currentUserGroupList.empty()){
        for(Group &group : g_currentUserGroupList){
            cout<<"群组：";
            cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            cout<<"群成员："<<endl;
            for(GroupUser &groupuser : group.getUsers()){
                cout<<groupuser.getId()<<" "<<groupuser.getName()<<" "<<groupuser.getState()
                    <<" "<<groupuser.getRole()<<endl;
            }
        }
    }
    cout<<"====================="<<endl;
}

// 处理注册的响应逻辑
void doRegResponse(json &responsejs){
    // json responsejs = json::parse(buffer); // 将字符串（网络接收到的原始数据）反序列化成json对象
    // 根据返回的json判断是否注册成功
    if(responsejs["errno"].get<int>() != 0){ // 注册失败
        cerr<<responsejs["name"]<<" is already exit,register error"<<endl;
    }else{ // 注册成功
        cerr<<responsejs["name"]<<" register success,userid is "<<responsejs["id"]
                <<",do not forget it!"<<endl; 
    }
}

// 处理登录的响应逻辑
void doLoginResponse(json &responsejs){
    // json responsejs = json::parse(buffer); // 子线程里已经反序列化过了
    if (responsejs["errno"].get<int>() != 0) // 登录失败
    {
        /* code */
        cerr<<responsejs["errmsg"]<<endl;
        g_isLoginSuccess = false;
    }
    else{ // 登录成功
        // 记录用户的id和name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        // 记录当前用户的好友列表信息
        if (responsejs.contains("friends"))
        {
            /* code */
            // 初始化
            g_currentUserFriendList.clear(); // 清空，防止有数据残留

            vector<string> vec = responsejs["friends"];
            for (string &str:vec)
            {
                /* code */
                json js = json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }
        
        // 记录当前用户的群组列表信息
        if (responsejs.contains("groups"))
        {
            /* code */
            // 初始化
            g_currentUserGroupList.clear(); // 清空，防止有数据残留

            vector<string> vec1 = responsejs["groups"];
            for (string &groupstr:vec1)
            {
                /* code */
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                vector<string> vec2 = grpjs["users"];
                for(string &userstr : vec2){
                    json js = json::parse(userstr);
                    GroupUser user;
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }
                g_currentUserGroupList.push_back(group); 
            }
            
        }

        // 显示登录用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线信息
        if (responsejs.contains("offlinemsg"))
        {
            /* code */
            vector<string> vec = responsejs["offlinemsg"];

            for(string &str : vec){
                json js = json::parse(str);
                // time + [id] + name + "said:" + xxx 

                if (ONE_CHAT_MSG == js["msgid"].get<int>()) // 聊天消息 获取的是离线消息的msg
                {
                    /* code */
                    cout<<js["time"].get<string>()<<" ["<<js["id"]<<"] "<<js["name"].get<string>()
                        <<" said:"<<js["msg"].get<string>()<<endl;

                }else{ // 群聊消息
                    /* code */
                    cout<<"群消息：["<<js["groupid"]<<"]:"<<js["time"].get<string>()<<" ["<<js["id"]<<"] "<<js["name"].get<string>()
                        <<" said:"<<js["msg"].get<string>()<<endl;
                }
            }
        }
        g_isLoginSuccess = true;
    }
}

// 接收线程（子线程）
void readTaskHandler(int serverfd){
    for(;;){
    //while(isMainMenuRunning){
        char buffer[1024] = {0}; // 只接收数据、显示数据
        int len = recv(serverfd,buffer,1024,0); // 默认情况下阻塞
        if (len == -1 || len == 0)
        {
            /* code */
            close(serverfd);
            exit(-1);
        }
        
        // 接收chatserver转发的数据（conn->send)，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msg_type = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msg_type) // 聊天消息
        {
            /* code */
            cout<<js["time"].get<string>()<<" ["<<js["id"]<<"] "<<js["name"].get<string>()
                <<" said:"<<js["msg"].get<string>()<<endl;
                continue;

        }
        if (GROUP_CHAT_MSG == msg_type) // 群聊消息
        {
            /* code */
            cout<<"群消息：["<<js["groupid"]<<"]:"<<js["time"].get<string>()<<" ["<<js["id"]<<"] "<<js["name"].get<string>()
                <<" said:"<<js["msg"].get<string>()<<endl;
            continue;
        }
        if(LOGIN_MSG_ACK == msg_type){ // 登录消息
            doLoginResponse(js); // 处理登录响应的业务逻辑
            sem_post(&rwsem); // 通知主线程，登录结果处理完成
            continue;
        }
        if(REG_MSG_ACK == msg_type){ // 注册消息
            doRegResponse(js);
            sem_post(&rwsem); // 通知主线程，注册结果处理完成
            continue;
        }
    }
}

// 相应的命令事件处理器，整型参数为文件描述符，字符串参数为要向服务器发送的用户输入的数据
void help(int fd = 0,string str = ""); // help函数本身不需要int和string的参数，但为了在map中保持类型一致添加
void chat(int,string);
void addfriend(int,string); 
void creategroup(int,string);
void addgroup(int,string);
void groupchat(int,string);
void loginout(int,string);

// 系统支持的客户端命令列表
unordered_map<string,string> commandMap = {
    {"help","显示所有支持的命令，格式help"},
    {"chat","一对一聊天，格式chat:friendid:message"},
    {"addfriend","添加好友，格式addfriend:friendid"},
    {"creategroup","创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup","加入群组，格式addgroup:groupid"},
    {"groupchat","群聊，格式groupchat:groupid:message"},
    {"loginout","注销，格式loginout"}
};

// 注册系统支持的客户端命令处理 当从用户输入截取到命令时，在map表里查找（用户可能会输入一些不正规、不合法的数据），如果有则执行相应方法
// 一个命令对应一个处理函数 
unordered_map<string,function<void(int,string)>> commandHandlerMap = {
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"loginout",loginout}
};

// 主聊天页面程序
void mainMenu(int serverfd){
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        /* code */
        cin.getline(buffer,1024);
        string commandbuf(buffer); // 初始化commandbuf
        string command; // 存储命令
        int idx = commandbuf.find(":"); // 除了helo和loginout命令，其余命令均带冒号
        if (idx == -1) // 未找到:=>helo和loginout命令
        {
            /* code */
            command = commandbuf; // 直接赋值
        }else{
            command = commandbuf.substr(0,idx); // 其他命令
        }
        auto it = commandHandlerMap.find(command); // 继续在commandHandlerMap查找 可能之前找到的不带冒号的是无效命令
        if (it == commandHandlerMap.end())
        {
            /* code */
            cerr<<"invalid input command!"<<endl;
            continue;
        }
        
        // 找到对应命令 
        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(serverfd,commandbuf.substr(idx+1,commandbuf.size()-idx));
    }
    
}

// C++ 中默认参数只能在声明或定义中的一处指定，不能两处都指定。
void help(int fd,string str){
    cout<<"show command list"<<endl;
    for (auto &p : commandMap)
    {
        /* code */
        cout<<p.first<<" : "<<p.second<<endl;
    }
    cout<<endl;
}

// 将用户输入的命令发送给服务器进行处理=>先封装成json数据格式，再序列化成字符串发送给服务器
void addfriend(int serverfd,string str){
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;

    string buffer = js.dump();
    int len = send(serverfd,buffer.c_str(),strlen(buffer.c_str()) + 1,0);
    if(len == -1){
        cerr<<"send addfriend msg error -> "<<buffer<<endl;
    }
}

void chat(int serverfd,string str){
    // friendid:message
    int idx = str.find(":");
    if (idx == -1)
    {
        /* code */
        cerr<<"chat command invalid!"<<endl;
        return;
    }

    int friendid = atoi(str.substr(0,idx).c_str());
    string message = str.substr(idx+1,str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string buffer = js.dump();
    int len = send(serverfd,buffer.c_str(),strlen(buffer.c_str()) + 1,0);
    if (len == -1)
    {
        /* code */
        cerr<<"send chat msg error -> "<<buffer<<endl;
    }
    
}

// creategroup:groupname:groupdesc
void creategroup(int serverfd,string str){
    int idx = str.find(":");

    if (idx == -1)
    {
        /* code */
        cerr<<"create group command invalid! "<<endl;
        return;
    }
    
    string groupname = str.substr(0,idx);
    string groupdesc = str.substr(idx+1,str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    js["time"] = getCurrentTime();

    string buffer = js.dump();
    int len = send(serverfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if (len == -1)
    {
        /* code */
        cerr<<"send create group msg err -> "<<buffer<<endl;
    }
    
}

// addgroup:groupid
void addgroup(int serverfd,string str){
    int groupid = atoi(str.c_str());

    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;

    string buffer = js.dump();
    int len = send(serverfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if (len == -1)
    {
        /* code */
        cerr<<"send add group msg err -> "<<buffer<<endl;
    }
}

// groupchat:groupid:message
void groupchat(int serverfd,string str){
    int idx = str.find(":");
    if (idx == -1)
    {
        /* code */
        cerr<<"groupchat command invalid!"<<endl;
        return;
    }
    
    int groupid = atoi(str.substr(0,idx).c_str());
    string message = str.substr(idx + 1,str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser .getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string buffer = js.dump();
    int len = send(serverfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if (len == -1)
    {
        /* code */
        cerr<<"send group chat msg err -> "<<buffer<<endl;
    }
}

// loginout
void loginout(int serverfd,string str){
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();

    string buffer = js.dump();
    int len = send(serverfd,buffer.c_str(),strlen(buffer.c_str()) + 1,0);
    if (len == -1)
    {
        /* code */
        cerr<<"send loginout msg error -> "<<buffer<<endl;
    }else{
        isMainMenuRunning = false; // 从mainMe12nu中退出循环，回到首页面
    }
 
}

string getCurrentTime(){
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}