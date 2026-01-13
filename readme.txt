2012-12-03 11:38:54
-------------------------------------------------------
bin               : third party programming
build             : makefile build shell etc ...
include           : third party head file
lib               : third party lib file
Makefile          : project makefile , 
readme.txt        : me
src               : your files , .c .cpp .h .hpp
test              : unit test files
tutorial_libezman : tutorial

make all for static library only
make dyn for Dynamic library only

Usage of library in linux(unix) platform：
	使用 -l 选项指定静态库和动态库的格式都是一样的，如果库文件名为libabc.so，那么就用 -labc 即可。
	链接时会去搜索这个库文件，如果不是系统库，那么你需要告诉链接器它的路径。有两种方法：一种是在参数中用 -L 选项指定库文件搜索路径，可以并列多个。例如：gcc -L /home/ddd -L/home/ddd/lib。 另一种方法是在环境变量中设置LD_LIBRARY_PATH 包含有你的动态库文件所在的路径。这个环境变量用在Sorlarise和Linux，如果你是在HP－UX下，是SHLIB_PATH。
	设置搜索路径-L 或者 指定环境变量$LD_LIBRARY_PATH后 在编译链接时附件-lxxx 就应该可以找到 libxxx.so(优先) 或者libxxx.a,如只需静态库, 那就删掉动态库,把静态库放到搜索路径上.

	example:
		LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/wujj/Dropbox/stock/trunk/code/library/libman/lib
    	export LD_LIBRARY_PATH
    or
	    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/wujj/Dropbox/stock/trunk/code/library/libman/lib
