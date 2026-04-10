set -x
rm -rf `pwd`/build/* 	# 当前路径下的build文件（编译的中间文件删除）
cd `pwd`/build &&
        cmake .. &&
        make
