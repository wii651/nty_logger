# nty_logger
* 简单易用

* 支持同步/异步两种日志记录方式

## quick start

第一步, 创建日志器

> 可以选择创建同步日志器, 在当前线程下同步记录日志
>
> ```c
> ntylogger_init(SYNC_LOG);
> ```
>
> 也可以选择创建异步日志器, 开辟专用的线程记录日志, 接收其它各线程的日志消息
>
> ```c
> ntylogger_init(ASYNC_LOG);
> ```

第二步, 书写日志

> ```c
> ntylog1("sync_module", "sync", "this is a %s test", "sync");
> ```
>
> 以上函数将执行
>
>  (1) 在执行路径下尝试创建logs/sync_module/[year]/[month]/sync-[day].log文件
>
>  (2) 打开(1)中文件, 追加一条记录, 内容为`[2021-08-21 22:51:43]--[sync_test.c:25:main()]--this is a sync test`
>
> ```c
> ntylog2("sync_module", "sync", "this is a %s test", "sync");
> ```
>
> 以上函数将执行
>
>  (1) 在执行路径下尝试创建logs/sync_module/[year]/[month]/sync-[day].log文件
>
>  (2) 打开(1)中文件, 追加一条记录, 内容为`this is a sync test`

第三步, 销毁日志器

> ```c
> ntylogger_destroy();
> ```

## 编译
以编译quick_start.c为例
```c
gcc async_test.c nty_inproc_mq.c nty_logger.c -pthread
```
如果在编译时添加`-D NOLOGMSG`选项, 可以禁止执行程序时标准输出调试信息
```c
gcc async_test.c nty_inproc_mq.c nty_logger.c -pthread -D NOLOGMSG
```

## 注意事项

* 日志器为全局单例对象, 只需要创建一次就可以全局使用

  >  因此, 在执行日志器销毁之前, 请不要重复创建

* 编译时请添加-pthread

