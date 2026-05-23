# IAP 启动与升级可靠性重构

日期：2026-05-23

## 启动判定

IAP 不再读取 `FLASH_ADDR_UPDATE_FLAG` 的 `0x00AB/0xFFFF` 半字作为启动门闩。

当前逻辑：

1. IAP 上电后读取 SRAM mailbox `0x20004FE0`。
2. mailbox 带 `magic`、反码、请求值和 CRC，全部匹配才认为 App 请求进入 IAP。
3. 如果请求有效，IAP 清除 mailbox 并留在升级模式。
4. 如果请求无效，IAP 只根据 App 向量表是否有效决定是否跳 App。

mailbox 是瞬时请求，只要求跨 `NVIC_SystemReset()` 保持；掉电后请求丢失也不会死机，App 仍可重新发送进入升级命令。

## 升级写入策略

升级稳定性不再依赖 Flash 标志，而是依赖 App 第 1 页最后写：

- 升级开始时擦除 App 第 1 页，立即让旧 App 向量表失效。
- 接收 App 第 1 页数据时只缓存到 RAM。
- 其他页正常写入 Flash。
- 串口完成命令或 CAN `END` 校验通过后，最后写 App 第 1 页。
- 第 1 页写入时，先写 offset 8 之后的数据，再写 Reset Handler，最后写 MSP。

这样升级中途断电时，App 向量表保持无效，下一次上电留在 IAP；只有完整镜像通过检查后才可能跳 App。

## 串口和 CAN

- 串口协议保持不变，仍走原 `0xFFFD/0xFFFE/0xFFFF` 升级命令。
- CAN 升级协议保持独立，走 `START/DATA/COMMIT/END`。
- 两者都调用同一个 `IapFlash` 底层写入流程，但协议状态机互不依赖。

