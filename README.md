### XMarketCenter
- 行情网关，采用插件架构，适配不同Broker柜台行情API，主要功能如下：
  - 收取行情数据；
  - 数据写入共享内存队列；
  - 行情数据转发至XWatcher监控组件。


- MarketReader：行情数据读取器，从共享内存队列读取行情数据。如果Segmentation fault，则需要设置操作系统的栈大小：
  ```bash
  ulimit -s 16384
  ```
