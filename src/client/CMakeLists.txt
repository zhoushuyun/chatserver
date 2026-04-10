# 定义了一个SRC_LIST变量，包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)

# 指定生成的可执行文件
add_executable(ChatClient ${SRC_LIST})

# 指定可执行文件链接时需要依赖的库文件
target_link_libraries(ChatClient pthread) # 客户端有一个读线程（接收服务器发送过来的消息响应）一个写线程(接收用户的输入，发送相应业务数据)