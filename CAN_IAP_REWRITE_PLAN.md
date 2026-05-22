# CAN-IAP 重构实施说明

本分支 `codex/can-iap-rewrite` 用于把旧串口 IAP 重构为 CAN-IAP。

## 固定目标

- IAP 起始地址：`0x08000000`。
- 第一阶段 App 起始地址：`0x08004800`。
- CAN 波特率：`250 kbit/s`。
- 默认节点：`node=1`。
- 升级协议与主仓库 `docs/BMS_CAN_IAP_PROTOCOL.md` 保持一致。

## 重构原则

- 任意失败都停留在 IAP，等待重新升级。
- IAP 禁止擦写自身区域。
- START 后清除 App 有效状态，END 校验成功后才写 App 有效状态。
- 每个 256B 块收到 `COMMIT` 后才写 Flash，写后必须回读校验。
- 跳 App 前关闭 CAN/USART/TIM/SysTick，清 NVIC pending，设置 `VTOR` 和 MSP。

## 第一批源码

- `Include/can_iap_protocol.h`
- `Source/can_iap_protocol.c`

第一批源码先实现协议常量、CRC、ID 构造、ACK/NACK 和 App 向量检查，后续接入 CAN 接收状态机、Flash 擦写和主循环。

