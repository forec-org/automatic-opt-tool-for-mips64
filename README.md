# MIPS64 指令集自动优化工具

* 目录 `Renaming` 为寄存器重命名相关优化，包含测试用 MIPS64 源码 `rename.s`，使用 `python2 renaming.py rename.s` 生成优化后代码 `rename_opt.s`。
* 目录 `Schedule` 为指令静态调度相关优化，包含测试用 MIPS64 源码 `schedule.s`，执行 `g++ -std=c++11 BlockSchedule.cpp` 生成可执行文件 `a.exe`，执行该文件生成优化后代码 `after_schedule.s`（待优化代码与 `schedule.s` 内容相同，已被内嵌入优化工具源码中以方便测试）。

## 许可证
此仓库中的代码受仓库中 `License` 文件声明的许可证保护。