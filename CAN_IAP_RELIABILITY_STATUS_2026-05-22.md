# CAN-IAP 可靠性实施记录

## 当前分区

| 区域 | 地址 | 说明 |
| --- | --- | --- |
| IAP | `0x08000000` | Bootloader 起始地址 |
| App | `0x08004800` | 当前 BMS App 起始地址 |
| App 写入上限 | `0x0801F800` | 预留升级标志页和睡眠标志页 |
| 升级标志 | `0x0801F800` | `0x00AB` 留在 IAP，`0xFFFF` 允许跳 App |
| 睡眠标志 | `0x0801FC00` | 不允许 CAN-IAP 擦写 |

## 已接入逻辑

- `START` 后立即写入 IAP 标志，升级中断或复位后继续停留在 IAP。
- `DATA` 使用扩展帧 `0x14000000 | (seq << 8) | node`，避免序号位和固定基址位重叠。
- CAN1 使用 PA11/PA12，CAN 收发器供电脚与当前 BMS App 保持一致为 PB4。
- 每 256B 作为一个提交块，`COMMIT` 校验块 CRC 后才擦写 Flash。
- Flash 写入只允许落在 `0x08004800` 到 `0x0801F800` 之间，写后逐半字回读校验。
- `END` 同时校验帧数、总长度、总 CRC 和 App 向量，通过后才写回 App 有效标志并复位。
- 跳 App 前关闭 SysTick/TIM/USART/CAN，清 NVIC pending，设置 `VTOR` 和 MSP。

## 仍需上板验证

- comm tool 真实 CAN 发送和 IAP ACK/NACK 时序。
- 升级中断复位后是否稳定停留 IAP。
- 最后一块非 256B、CRC 错误、丢帧、重复帧、BusOff 恢复场景。
