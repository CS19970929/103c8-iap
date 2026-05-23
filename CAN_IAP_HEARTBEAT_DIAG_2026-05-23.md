# CAN-IAP 心跳诊断说明

## 修正点

IAP 工程当前使用 `SYSCLK=72MHz`，`PCLK1=36MHz`。CAN 时序为 `SJW=1tq, BS1=5tq, BS2=2tq`，总时间片为 8tq，因此 250k 的 prescaler 必须是 `18`：

`36MHz / 8tq / 18 = 250000 bit/s`

原 `Prescaler=4` 会得到约 1.125Mbps，无法和 250k 的 comm tool、BMS App 正常通信。

## 心跳帧

- ID：标准帧 `0x05F`
- 周期：1000ms
- 数据：
  - Byte0：`0x49`，字符 `I`
  - Byte1：`0x41`，字符 `A`
  - Byte2：CAN-IAP 协议版本
  - Byte3：IAP node id
  - Byte4：IAP 状态，`0=IDLE, 1=RECEIVING, 2=DONE, 3=ERROR`
  - Byte5：最近收到的 IAP 命令
  - Byte6：最近错误码，`0=OK`
  - Byte7：心跳递增序号

示例：`49 41 01 01 00 00 00 10` 表示 IAP 已运行、node=1、空闲、无错误。

## 测试方法

1. 烧录本 IAP 到 `0x08000000`。
2. 让 BMS 停在 IAP。
3. CAN 通讯盒设置为 250k、normal/active 模式。
4. 观察标准帧 `0x05F` 是否每秒出现。
5. 如果看到 `0x05F`，说明 BMS IAP CAN 位时序、收发器使能、TX 路径基本正常；再执行 comm tool 升级。
6. 如果看不到 `0x05F`，优先检查 IAP 是否真的运行、CANH/CANL/GND、终端电阻、通讯盒是否 listen-only、PB4/`CMNT_EN` 收发器使能电平。

## 兼容性

心跳帧只用于诊断，不参与 CAN-IAP 升级协议。发送失败会取消 TX mailbox，避免无 ACK 时占满邮箱影响后续升级。

