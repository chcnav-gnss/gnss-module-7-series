# FirmwareUpgrade

`FirmwareUpgrade` 目录提供了一个基于 Linux 的 CHCNAV 7 系列 GNSS 模组固件升级参考 Demo，面向 OEM 客户、系统集成商以及需要进行主机侧串口升级集成的开发人员。

## 内容组成
- 升级流程与控制入口：`UpgradeEntry.c`、`UpgradeM7xx.c`
- 升级协议处理：`UpgradeProtocol.c`、`UpgradeProtocol.h`
- 串口通信封装：`UartCommon.c`、`UartCommon.h`
- 完整性校验辅助：`CRCGenerate.c`、`md5.c`
- 构建文件：`Makefile`、`build.sh`

## 构建前提
- Linux 构建环境
- GCC 工具链
- `make`
- 可访问目标设备对应的串口节点
- 与目标模组匹配的 `.pkg` 固件包

## 构建方式
### Linux 原生编译
```bash
cd examples/FirmwareUpgrade
make
```

默认生成的可执行文件为：
```bash
update_m7xx
```

### 交叉编译
如果需要交叉编译，请根据实际工具链修改 `Makefile` 中的 `CC` 设置。

仓库同时提供了 `build.sh`，可作为嵌入式 Linux 交叉编译环境下的参考构建脚本。

## 命令使用说明
该工具仅适用于 Linux 平台。

命令格式：
```bash
./update_m7xx -o -d <device_path> -b <baudrate> -p <package_path> -t <transfer_size> -r -c -f -u
```

命令示例：
```bash
./update_m7xx -o -d /dev/ttySP1 -b 921600 -p Enc_M722_V2.1.31.1.pkg -t 2048 -r -c -f
./update_m7xx -o -d /dev/ttySP1 -b 921600 -p Enc_M722_V2.1.31.1.pkg -t 2048 -r -c -f -u
```

## 参数说明
- `-o`：输出调试信息；不带该参数时为静默模式
- `-d`：目标设备节点
- `-b`：通信波特率；支持 `9600`、`19200`、`115200`、`460800`、`921600`、`1500000`、`3000000`
- `-p`：固件包路径
- `-t`：单帧传输的有效负载长度；支持 `1024`、`2048`、`4096`、`8192`、`16384`、`32768`
- `-r`：升级完成后立即重启模组
- `-c`：开启快速升级模式
- `-f`：强制升级；忽略版本比较并支持版本回退
- `-u`：开启远端设备升级模式，适用于 LoRa 透传等中继升级场景

## 推荐配置
- 推荐使用 `-t 2048` 作为较均衡的传输长度
- 推荐开启 `-r`，减少升级后手动复位操作
- 推荐开启 `-c`，缩短升级耗时
- 需要支持版本回退时，推荐开启 `-f`
- `-u` 仅用于远端中继升级场景，直连串口升级通常不需要开启

## 使用注意事项
- 升级前请确认固件包与目标模组型号完全匹配。
- 升级前请检查串口权限、串口占用状态以及连接稳定性。
- 建议先在受控环境中完成完整升级验证，再用于现场部署。
