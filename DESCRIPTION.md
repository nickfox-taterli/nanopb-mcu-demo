# STM32 + PC Protobuf 串口通信示例

本示例演示如何使用 **nanopb**(Protocol Buffers 的 C 语言实现)在 STM32 MCU 与 PC 之间通过串口进行结构化双向通信.

## 项目组成

| 工程 | 说明 |
|------|------|
| `c092_hello` | MCU 端 - STM32C091RC,UART 中断收发,nanopb 编解码 |
| `pbchat-host` | PC 端 - Linux C 程序,串口读写 + 终端输入,nanopb 编解码 |

## 通信协议

### 帧格式

```
┌──────────┬──────────────────┐
│ 2 字节   │ N 字节           │
│ 长度头   │ protobuf payload │
│ (小端)   │                  │
└──────────┴──────────────────┘
```

- **长度头**:2 字节无符号小端序,表示后续 payload 字节数
- **Payload**:`ChatMsg` 的 protobuf 序列化数据

### Proto 定义

```protobuf
message ChatMsg {
  required uint32 seq  = 1;  // 序号
  required string from = 2;  // 发送方标识
  required string text = 3;  // 消息内容
  optional bool   ack  = 4;  // 确认标记
}
```

## 工作流程

1. PC 连接 MCU 串口(115200 8N1),发送 `hello from pc`
2. MCU 收到后解码 protobuf,回复 ack
3. 用户在 PC 终端输入文本,逐行发送给 MCU
4. 双方基于相同的帧格式和 proto 定义,互发互收

## 构建与运行

### MCU 端

```bash
# 依赖:arm-none-eabi-gcc, cmake, openocd
cmake -S . -B build
cmake --build build -j
# 烧录(OpenOCD + ST-Link)
openocd -f .vscode/openocd.cfg -c "program build/c092_hello.elf verify reset exit"
```

### PC 端

```bash
# 依赖:gcc, cmake
./scripts/gen_proto.sh
cmake -S . -B build
cmake --build build -j
./build/pbchat-host /dev/ttyACM0
```

## 关键技术点

- **nanopb**:适用于资源受限 MCU 的 protobuf 编解码库,无需动态内存分配
- **帧同步**:2 字节长度前缀 + payload,接收端按长度截取,无需特殊分隔符
- **中断收发**:MCU 端使用 UART 中断逐字节接收,状态机解析帧头和 payload
- **select 多路复用**:PC 端同时监听串口和终端输入,实现异步收发
