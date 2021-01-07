


```
-- ------------------------Address Info-------------------
-- 0x00 : Control signals
--        bit 0  - ap_start (Read/Write/COH)
--        bit 1  - ap_done (Read/COR)
--        bit 2  - ap_idle (Read)
--        bit 3  - ap_ready (Read)
--        bit 7  - auto_restart (Read/Write)
--        others - reserved
-- 0x04 : Global Interrupt Enable Register
--        bit 0  - Global Interrupt Enable (Read/Write)
--        others - reserved
-- 0x08 : IP Interrupt Enable Register (Read/Write)
--        bit 0  - enable ap_done interrupt (Read/Write)
--        bit 1  - enable ap_ready interrupt (Read/Write)
--        others - reserved
-- 0x0c : IP Interrupt Status Register (Read/TOW)
--        bit 0  - ap_done (COR/TOW)
--        bit 1  - ap_ready (COR/TOW)
--        others - reserved
-- 0x10 : Data signal of fb
--        bit 31~0 - fb[31:0] (Read/Write)
-- 0x14 : reserved
-- 0x18 : Data signal of col
--        bit 31~0 - col[31:0] (Read/Write)
-- 0x1c : reserved
-- 0x20 : Data signal of x1
--        bit 15~0 - x1[15:0] (Read/Write)
--        others   - reserved
-- 0x24 : reserved
-- 0x28 : Data signal of y1
--        bit 15~0 - y1[15:0] (Read/Write)
--        others   - reserved
-- 0x2c : reserved
-- 0x30 : Data signal of x2
--        bit 15~0 - x2[15:0] (Read/Write)
--        others   - reserved
-- 0x34 : reserved
-- 0x38 : Data signal of y2
--        bit 15~0 - y2[15:0] (Read/Write)
--        others   - reserved
-- 0x3c : reserved
-- (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)
```


