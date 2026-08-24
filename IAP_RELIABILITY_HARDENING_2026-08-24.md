# IAP reliability hardening

Date: 2026-08-24
Branch: `codex/iap-reliability-hardening`
Base: `can-iap-common`

## Compatibility constraints

The following contracts are intentionally unchanged:

- Serial upgrade protocol and command addresses remain unchanged.
- CAN-IAP protocol version, CAN IDs, command values, payload fields, CRC16 algorithm and 250 kbit/s behavior remain unchanged.
- IAP start address, APP start address, APP limit address, update/sleep flag addresses remain unchanged.
- `BootCtrl_ShouldJumpToApp()` and `IAP_To_APP_Jump()` behavior are unchanged.
- The first APP page is still invalidated at upgrade start and committed last after validation.

Because of these constraints, this branch intentionally does **not** add an APP pending/confirm boot state, change image CRC16 to CRC32, or extend the serial protocol with a new image-size/image-CRC field.

## Implemented hardening

### Flash transaction layer

`Source/iap_flash.c` now:

- tracks erased APP pages per upgrade session;
- erases every page before first programming of that page;
- splits writes at STM32F103 1 KiB Flash page boundaries;
- supports CAN blocks that cross a Flash page boundary;
- supports odd-length final data by holding one pending byte until the paired byte arrives or the image is finalized;
- preserves the legacy serial fixed-block placement behavior;
- retains the existing first-page-last commit strategy;
- verifies every programmed halfword by readback;
- keeps owner arbitration between serial and CAN sessions;
- clears all in-RAM Flash-session state on abort or successful finish.

Power loss before final first-page commit therefore still leaves the APP vector invalid and the MCU in IAP on the next reset.

### Serial IAP recovery

`Source/iap_upgrade.c` now:

- releases the serial Flash owner on every fatal serial-session error;
- releases the owner on timeout;
- allows a fresh serial connection after a failed serial session;
- avoids a failed serial session permanently blocking CAN recovery;
- requires the serial session to actually be in receiving state before accepting COMPLETE.

No serial frame format, command address, byte-count interpretation or response format is changed.

### CAN-IAP recovery and idempotency

`Source/can_iap.c` now:

- releases the CAN Flash owner on fatal Flash/validation/timeout errors;
- allows a new START after a failed CAN session;
- records the last successfully committed block;
- treats an identical retransmitted COMMIT as idempotent and re-sends ACK without writing Flash again;
- rejects a partial non-final COMMIT that would silently discard received bytes;
- still permits up to seven padding bytes in the final 8-byte CAN DATA frame group;
- keeps the existing accept-all hardware CAN filter to avoid changing deployed node-selection/protocol behavior.

No CAN ID, command, DLC, payload field, CRC16 rule, node rule or heartbeat frame format is changed.

## Required regression / fault-injection tests

### Existing behavior

1. Existing serial upper-computer software upgrades an existing known-good APP successfully without any host change.
2. Existing CAN upgrade tool upgrades the same APP successfully without any host change.
3. Normal power-on with a valid APP still jumps directly to the APP.
4. APP request-to-IAP mailbox behavior remains identical.

### Flash boundaries

5. CAN image sizes: 8 B, 255 B, 256 B, 257 B, 1023 B, 1024 B, 1025 B and maximum allowed size.
6. A CAN COMMIT whose payload crosses a 1 KiB Flash boundary.
7. Final firmware length is odd.
8. Final CAN block uses 1 to 7 padding bytes.
9. Attempted write beyond `CAN_IAP_APP_LIMIT_ADDR` is rejected and IAP remains recoverable.

### CAN fault injection

10. Lose one DATA frame: device returns sequence error and does not advance Flash commit.
11. Duplicate one DATA frame: device does not silently accept an incorrect sequence.
12. Lose COMMIT ACK and resend the same COMMIT: device re-sends ACK and does not erase/program the block again.
13. Send wrong block CRC: block is not written.
14. Send wrong firmware CRC at END: first APP page is not committed.
15. Reset/power-off during DATA reception.
16. Reset/power-off during a non-first-page Flash write.
17. Reset/power-off immediately before END.
18. Bus-Off during an upgrade and recovery with ABOM enabled.
19. CAN timeout followed by a fresh CAN START.
20. CAN fatal error followed by a serial upgrade, proving owner release.

### Serial fault injection

21. Serial timeout followed by a fresh serial CONNECT.
22. Invalid block length followed by a fresh serial CONNECT.
23. Serial Flash failure/error followed by CAN START, proving owner release.
24. Reset/power-off after CONNECT, during first block, during later block and before COMPLETE.
25. COMPLETE with no received block is rejected.

## Remaining engineering constraints

The following limitations remain by explicit compatibility requirement:

- Serial protocol has no independent expected whole-image size/CRC field, so its end-to-end image-integrity guarantee cannot be made equivalent to CAN without changing the serial protocol.
- Boot decision has no APP boot-confirmation/rollback state. A structurally valid APP that immediately crashes can still reboot repeatedly because changing that behavior would modify the current APP jump logic.
- CAN whole-image integrity remains CRC16 because changing it to CRC32 would change the deployed CAN protocol.

These are compatibility tradeoffs, not hidden implementation omissions.
