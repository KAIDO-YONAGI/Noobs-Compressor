# 代码框架：


# 数据块模块
>文件位置：/DataBlocks  
结构：  
A：字节流数据缓冲块 /DataBlocks/DataBlocksManage.h

该头文件包含：一个基础的数据块列表类DataBlocks，一个数据块管理器类。数据块管理器实现了数据传输接口，内部包含两个DataBlocks。

**（注意事项）：**  
在一个模块使用传输接口取出io块，并完成所有工作后，需要调用done()，将in块清空，并轮转in/out块——下一次的in块是原out块。

# 线程池
>文件位置：/ThreadPool  
结构：  
**A: 线程池类** /ThreadPool/ThreadPool.h  
**B：线程类** /ThreadPool/_Thread.h  
**C：任务队列管程** /ThreadPool/MonitorTaskQueue.h  

该模块接口为A的接口

## A: 线程池类 
>/ThreadPool/ThreadPool.h

定义一个线程池类。线程的创建和销毁均被封装在内部，每个线程可拥有一个名称，这些名称通常可由持有线程池的进程/对象来管理，通过对名称的指定，可以完成如下操作：

**（接口）：**  
new_thread：创建一个命名线程  
del_thread：删除一个命名线程  
add_task：向命名线程添加任务  
get_thread_nums：获取当前池内线程数

**注意事项：**  
（1）add_task的任务函数应为void()，使用function函数包装器包装。如果需要传参与返回值的函数，可使用lamda表达式将其包装，交给add_task。在本项目中还有一个特殊需求，即进程需要阻塞等待一批任务执行完毕，我们使用std::packaged_task实现，将lamda用任务包包装，获得future对象等待任务执行完毕。  
（2）get_thread_nums只能在池由单线程管理的情况下才可使用。
（3）在堆上创建线程池对象。

关于线程与任务队列的细节请往下阅读

## B：线程类
>/ThreadPool/_Thread.h

定义一个线程类，被设计成一个线程一个任务队列，创建该类对象即创建一个线程，该线程运行一个不断从任务队列中取出可调用对象并执行的函数。  

**（接口）：**  
add_task：将可调用对象加入任务队列。

## C：任务队列管程
>/ThreadPool/MonitorTaskQueue.h

定义一个管程类管理任务队列，对任务队列的操作通过管程进行，一个时刻只能有一个线程进入管程。除了锁，该类还封装了条件变量，当没有任务时线程休眠，或者有任务时唤醒线程。

**（接口）：**  
add_task：将可调用对象加入任务队列后，尝试唤醒线程。
get_task：若队非空取出可调用对象,否则线程休眠。

# 压缩模块
>文件位置： /CompressionModules  
结构：  
A：赫夫曼压缩模块 /CompressionModules/heffman  
B：赫夫曼数据结构 /CompressionModules/hefftype  

## A：赫夫曼压缩模块
>/CompressionModules/heffman

该模块由一个算法核心，数个功能模块组成。

### heffman--算法核心
>/CompressionModules/heffman/Heffman.h

这里有heffman压缩的主要数据处理算法。包括：  
1）统计频率  
2）合并频率表  
3）生成heffman树结构  
4）压缩编码  
5）解码解压  

**注意事项：**  
1）对于压缩，数据编码后会进入bit处理器，由它合成字节流输出。对于解压，接受的数据会进入bit处理器，由它解析成bit流。  

### heffman--编码表保存/加载模块
>/CompressionModules/heffman/GenHeffcodeTab.h
>/CompressionModules/heffman/LoadHeffcodeTab.h

该模块可被调度。负责编码表的输入输出。即——heffman树序列化后输出，接受后反序列化、同时构建heffman树。

### heffman--频率统计模块
>/CompressionModules/heffman/GetFreq.h

该模块可被调度。负责统计字符频率。支持多线程。

**注意事项：**  
//FIXME: 当前是数据块列表有两个及以上块时会触发多线程。更好的做法的可选择模式，存在单线程循环处理多块的解法。

### heffman--压缩/解压模块
>/CompressionModules/heffman/DoEncode.h
>/CompressionModules/heffman/DoDecode.h

该模块可被调度。支持多线程。


## B：赫夫曼数据结构
>/CompressionModules/hefftype/hefftype.h

赫夫曼算法所需的一些数据结构。包括哈希表、最小堆、树路径栈、二叉树。  
哈希表是字符到字符信息（频率、编码长度、编码结果）的映射。  
最小堆辅助生成heffman树。  
树路径栈辅助生成编码。



# 调度模块：
>文件位置：/Schedule  
结构：  
**A: 调度模块** /Schedule/Schduler.h  
**B1：策略模块** /Schedule/include     
**B2: 三个接口（抽象基类）** /Schedule/include  

## A: 调度模块

## B1：策略模块

## B2：接口——数据传输接口
>Schedule/include/Datacmnctor.h

用于传递数据块。三个纯虚函数：  
1) get_input_blocks()：取出in块列表（原料）
2) get_output_blocks()：取出out块列表（结果）
3) done()：补充尾工作

## B2：接口——行为接口
>Schedule/include/Worker.h

将不同的数据处理模块抽象成可被调度的工人，实现具体的工作。使用该接口的类实现具体的work方法。

## B2：接口——策略接口
>Schedule/include/Strategy.h

不同策略类的抽象，决定如何运转一组数据处理对象。使用该接口的策略类实现具体的策略方法。
