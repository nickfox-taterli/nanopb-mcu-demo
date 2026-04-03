# pbchat-host

PC 端 protobuf 串口示例（C + nanopb），配套 MCU 端 `c092_hello` 工程。

## 构建

```bash
./scripts/gen_proto.sh
cmake -S . -B build
cmake --build build -j
```

## 运行

```bash
./build/pbchat-host /dev/ttyACM0
```

启动后会先发送一帧 `hello from pc`，然后可在终端输入文本逐行发送。
