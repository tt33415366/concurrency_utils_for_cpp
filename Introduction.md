## 需求
用C++语言实现一个库， 要求该库实现一个无锁队列， 并基于该无锁队列实现一个线程池。
## 要求
- C++11
- 阅读 [C++ Concurrency In Action](<C++ Concurrency in Action, Second Edition - Anthony Williams.epub>) 书籍， 使用该书籍的知识工作
- 无锁队列建议使用RingBuffer作为基础数据结构
- 需要同步输出单元测试
- 编写benchmark代码， 基于benchmark结果输出无锁队列和线程池的性能参数
- 使用Linux内核代码风格
### 可选高级要求
- 线程池支持工作窃取， 通过宏控制， 是否开启该项支持