# arraySum
 CPC2020 赛前培训 - 示例程序：多项式求和

 宏定义列表：
 1. src/sum.c :
	"#define ATHREAD"	- 众核并行开关
 2. inc/swarg.h :
	"#define USE_SIMD"	- SIMD向量化开关
	"#define REG_COM"	- 寄存器通信开关

 提交命令：
 1. run.sh :		普通提交命令
 2. gprof_run.sh :	带 gprof 采样的提交命令
 3. perf_run.sh :	带 Jperf 采样的提交命令
