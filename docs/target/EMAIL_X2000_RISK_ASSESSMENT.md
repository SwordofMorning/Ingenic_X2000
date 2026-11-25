**发件人**: 肖劲涛 <jintao.xiao@vitalchem.com>  
**收件人**: 杨厚逸 <houyi.yang@votinfrared.com>  
**抄送**: sheying.yan@votinfrared.com、gang.zhao@votinfrared.com  
**主题**: 【风险评估】Minilite-X2000 平台移植与全负载场景可行性核查及折中方案汇报  

---

各位领导，您好：

围绕 Minilite 平台向君正 **X2000**（核心板原理图器件符号印为 `X2000`，样机实际贴装为片内封装 **512 MB SiP LPDDR3** 的大容量型号 **`X2000H`**）迁移及《x2000评估验证，仿真跑全负载场景》[[1]](#ref-1) 所列各项技术指标，我们近期结合官方芯片手册、外围器件规格书、`A-PCB-MB-TSIN01` 核心板与共用 `JK` 子板原理图以及 Linux 5.10 SDK 底层驱动源码，完成了全链路物理边界推演与软硬件逐项核查。

前期评估选型阶段关注 X2000 系列，主要是基于其官方规格标称支持最高 `1920×1080@40Hz` 显示输出与 `1080p30` 硬件编码 [[2]](#ref-2)，期望通过原生驱动 `1440×1080` 高分辨率微显示屏，进一步提升枪瞄十字分划与 UI 的边缘锐利度和瞄准精度，且实贴 `X2000H` 具备充裕的 `512 MB` 运行内存空间。这一选型初衷在单芯片基础指标层面是完全合理的；但在将 **X2000 硬件接口物理形态（2-Lane DSI、无 H.265、16-bit 内存总线位宽）** 与 **现有共用子板外围器件边界** 做交叉核算后，发现部分原定满规格指标触及了芯片与屏幕接口的硬性物理上限，同时原理图层面存在若干首次上电前必须处理的电平匹配隐患。

为便于团队提前对齐预期并决策评估基线，现将核查结论按 **“明确无法按原规格实现项”、“补救与折中落地方案”、“其他潜在高风险事项”** 及 **“已确认风险可控项”** 四部分客观汇报如下：

---

### 一、明确无法按原规格实现的内容（芯片与屏幕硬性物理边界）

以下 4 项限制由芯片硅片硬件规格或物理传输协议决定，无法单纯依靠软件优化突破：

1. **显示输出（VO）无法实现 MIPI-DSI `1440×1080@50Hz` 原生点对点直出（直接影响屏显十字分划原生锐度预期）**
   - **物理成因**：虽然 X2000 的 MIPI D-PHY 发送端单通道速率较高（芯片标称可配高带宽屏跑 `1920×1080@40Hz` [[2]](#ref-2)），但 **X2000 硬件仅封装了 2 条 MIPI-DSI 数据通道（2-Lane），且无 VESA-DSC 视觉无损压缩发送硬件** [[3]](#ref-3)[[4]](#ref-4)[[5]](#ref-5)；受此制约，本项目共用的视涯 `SY049WDM01` 微显示屏（接收端单通道物理上限严格限制为 **`1.0 Gbps/Lane`（`UI ≥ 1.0 ns`）** [[6]](#ref-6)）无法启用其片内 DSC 解码器，只能工作于未压缩模式，而该模式下屏幕仅支持 24-bit `RGB888` 输入（不支持 `RGB565` 等低位宽降带宽格式）[[7]](#ref-7)。
   - **带宽核算**：此外，既有平台验证表明该屏幕内部电荷泵必须锁定输出水平行频 **`f_H ≈ 74.88 kHz`**（对应 50Hz 下总行数 `VTOTAL = 1498`），否则上电 1.5 秒即触发欠压白闪并于 30 秒保护关断 [[8]](#ref-8)。在此硬性行频约束下，原生点对点输出 `1440×1080@50Hz`（`1568 × 1498 × 50Hz`）所需像素时钟达 `117.44 MHz`，分摊至 2 条通道上的**含消隐连续像素速率即达 `1.41 Gbps/Lane`**（纯有效像素速率为 `0.93 Gbps/Lane`），计入 MIPI Burst 包开销与切换裕量（`1.05× ~ 1.20×`）后实需 **`1.48 ~ 1.69 Gbps/Lane`**，超出屏幕接收端 `1.0 Gbps/Lane` 物理极限 **41% ~ 69%** [[9]](#ref-9)（即便完全忽略锁行约束按极限最小消隐估算，含消隐连续速率仍超 `1.06 Gbps/Lane`、Burst 峰值约 `1.27 Gbps/Lane`），在 `-20°C ~ +70°C` 全温域下必然发生高速眼图闭合与闪屏失步。

2. **编码模块不支持 H.265（HEVC）硬件编码**
   - **物理成因**：经核查芯片数据手册 [[2]](#ref-2)、编程手册 [[10]](#ref-10)[[11]](#ref-11) 及内核 VPU 驱动 [[12]](#ref-12)，X2000 内置的 `Helix` 硬件视频编码引擎**仅支持 H.264（AVC）编码与 JPEG/MJPEG 编解码** [[10]](#ref-10)[[12]](#ref-12)（另一独立单元 `Felix` 仅支持 H.264 解码 [[11]](#ref-11)），硅片层面无任何 H.265 编码硬件逻辑；双核 1.2 GHz CPU [[13]](#ref-13) 亦无法实时软编 `1440×1080` H.265。因此需求中的“H.264 / H.265 编码”[[1]](#ref-1) 仅能落地 **H.264** 与 **MJPEG**。

3. **VPU 标称频率（300 MHz）下无法满规格并发跑通“双路 `1440×1080@25fps` + `1fps` 抓拍”，亦无法单路跑满 `1440×1080@50fps`**
   - **物理成因**：X2000 的 `Helix` VPU 与片上高速外设共用 `AHB0` 总线时钟（标称 **`300 MHz`**）[[14]](#ref-14)，官方标称最高编码能力为 **`1920×1080@30fps`** [[2]](#ref-2)（折合像素吞吐 **`62.2 MPix/s`**，按 16×16 宏块计为 **`244,800 宏块/秒`**），多路编码与 JPEG 抓拍在硬件上共用同一引擎逐帧串行分时复用 [[12]](#ref-12)。
   - **算力缺口**：
     - 主评估需求“通道 1 带 UI `1440×1080@25fps` + 通道 2 纯视频 `1440×1080@25fps` + 通道 3 `1440×1080@1fps` 抓拍”（合计 **51 fps**）[[1]](#ref-1) 总负载达 **`79.3 MPix/s`（`312,120 宏块/秒`）**，超出 300 MHz 标称上限 **27.5%** [[15]](#ref-15)；
     - 备选全负载场景“单路 `1440×1080@50fps` 编码”[[16]](#ref-16) 负载达 **`77.8 MPix/s`（`306,000 宏块/秒`）**，超出标称上限 **25.0%**（300 MHz 下单路 `1440×1080` 理论硬上限恰好为 **`40.0 fps`**；这也与前期摸底 `1280×1024` 超过 40fps 后因逼近算力上限并叠加默认用户态码控延迟、单缓冲阻塞及缩放器时钟偏低缺陷而出现 UV 色度异常的现象相印证 [[15]](#ref-15)[[16]](#ref-16)[[17]](#ref-17)）。

4. **UVC 无法直出未压缩 `RGB24`，且共用连接板上的 `MS1836S`（HDMI 转 CVBS）模拟视频输出无法使用**
   - **UVC 限制**：X2000 仅具备 USB 2.0 High-Speed 接口 [[18]](#ref-18)（批量传输协议有效上限约 `38 ~ 42 MB/s`，同步传输上限仅 `24.58 MB/s` [[19]](#ref-19)[[20]](#ref-20)），而未压缩 `1440×1080@25fps RGB24` 带宽高达 **`116.6 MB/s`**（即便 `640×512@50fps RGB24` 亦达 `49.2 MB/s`），超出物理线速；同时 Windows 10/11 原生免驱驱动 `usbvideo.sys` 不支持未压缩 `RGB24` 直接枚举 [[20]](#ref-20)。
   - **CVBS 限制**：共用连接板 `CNT_VA` 上的 `MS1836S` 转换芯片仅接收 **HDMI/TMDS 差分信号输入** [[21]](#ref-21)[[22]](#ref-22)（原海思平台由原生 HDMI TX 驱动）；而 **X2000 无原生 HDMI TX 接口** [[3]](#ref-3)（核心板 `CON5` 对应引脚实为 3.3V 并口 LCD 信号 [[23]](#ref-23)），信号体制完全不兼容。

---

### 二、针对上述限制的补救与折中落地方案

针对上述物理边界，我们制定了在不改动主芯片前提下可立即落地的工程折中方案，具体对比如下：

| 受限模块 | 原评估需求目标 [[1]](#ref-1) | 物理瓶颈根因 | 推荐工程折中与补救方案 |
| :--- | :--- | :--- | :--- |
| **MIPI-DSI 显示与十字分划** | `1440×1080@50Hz` 原生直出 | 2-Lane DSI（无 DSC）+ 屏端 `1.0 Gbps/Lane` 上限 + `74.88 kHz` 锁行（直出需 `1.41~1.69 Gbps/L`） | **启用屏幕内置 `1.5×` 硬件放大**：X2000 输出 `960×720@50Hz`（Burst 线速率约 `780.0~781.8 Mbps/L` 合规，输出行频 `74.84 kHz` 精准锁行）；**录像/抓拍通道独立按 `1440×1080` 原生精度叠加分划**，确保光轴零偏移 |
| **视频编码格式与多路并发** | H.264/H.265，双路 `1440P@25` + `1fps` 抓拍（或单路 `50fps`） | 无 H.265 硅片；300 MHz VPU 上限 `62.2 MPix/s`，满配超标 **27.5%** | 统一采用 **H.264 + MJPEG**；提供 **300 MHz 稳态合规模式**（主路保持 `1440P@25`，辅路按需开启或降为 `960×720@25` / `1440P@14fps`）与 **375 MHz 静态超频压测模式** 供评估决策 |
| **UVC 与模拟 CVBS 输出** | UVC 输出 YUV/RGB/H.264；复用连接板 CVBS | USB 2.0 带宽上限 `~42 MB/s`；X2000 无 HDMI TX 驱动 `MS1836S` | UVC 采用 **HS-Bulk 批量传输 + Win10/11 免驱复合方案**，输出 `640×512@50 YUY2` + `1440P@25 MJPEG`（PC 端解码还原 RGB）+ `1440P@25 H.264`；软件关断 `MS1836S` 减少无效电流 `>350 mA`（典型约 `428 mA / 1.1 W`） |

1. **显示与分划清晰度补救方案（客观评估视觉折损与缓解手段）**
   - **主推稳态配置（`960×720@50Hz` + 屏端 `1.5×` 硬件缩放）**：通过全 I2C 序列配置 `SY049WDM01` 开启双通道模式与内置 `1.5×` 硬件放大器（`SCACTRL 0x6900=0x0002`）[[24]](#ref-24)，由 X2000 输出 `960×720@50Hz`（含消隐连续速率 `651.4 Mbps/Lane`，1.2× Burst 实配线速率 **`780.0 Mbps/Lane`**，理论 50.00Hz 对应 **`781.8 Mbps/Lane`**，较 `1.0 Gbps/Lane` 留有约 **22%** 充裕安全裕量），输入行频 `49.89 kHz` 经屏端 `1.5×` 倍频后输出扫描行频精准落在 **`74.84 kHz`**，完美契合电荷泵锁行要求且不触碰面板 `90 Hz` 扫描上限 [[9]](#ref-9)[[24]](#ref-24)。同时预留规格书标准 `1280×720 → 1920×1080`（左右黑边居中）作为兼容性保底，并提供 **`1080×810@50Hz`（屏端 `1.33×` 缩放，Burst 速率 `976.3 Mbps/Lane`）** 作为样机极限清晰度对比摸底档位 [[9]](#ref-9)[[24]](#ref-24)。
   - **对十字分划观感的客观影响**：前端红外机芯原始分辨率为 `640×512`，热成像底图经二次或单次放大至 `1440×1080` 在主观观感上几乎无损；但**目镜端的图形层（UI 与枪瞄十字分划）物理画布降为 `960×720`**（原 `496×496` 分划图在屏显端按 `0.667×` 绘制为 `330×330` 后再由屏幕硬件 `1.5×` 插值放大），因此**屏显单像素细十字线及小字号边缘会经历一次硬件插值重采样，锐利度确实弱于原生 `1440×1080` 点对点直出**。工程上将通过离线预烘焙像素对齐的抗锯齿分划图元并在样机上实测对比 `1.5×`（`960×720`）与 `1.33×`（`1080×810`）档位，尽可能收敛视觉差距。
   - **录像/抓拍高清分划与“零光轴偏移”保障**：软件采用统一的 `1440×1080` 逻辑光轴坐标系（中心 `720, 540`）双尺度独立渲染——屏显侧渲染 `960×720` 画布，而**录像与抓拍通道在内存中独立按 `1.0×` 原生精度将 `496×496` 高清分划直接叠加至 `1440×1080` 视频帧**，确保录制的 `.mp4`、`.jpg` 抓拍及推流画面保持原生 1440P 锐利度，且屏显瞄准中心与录像十字中心严格一致、零像素偏移 [[9]](#ref-9)[[25]](#ref-25)。

2. **编码格式与多路算力调配方案**
   - **稳态合规模式（300 MHz 标称频率，推荐作为量产基线）**：主码流（通道 1，带高清 UI/分划，承担持续录制、5s 预录及主路 RTSP）保持满规格 **`1440×1080@25fps` H.264**，抓拍通道（通道 3）保持 **`1440×1080@1fps` MJPEG**，日常常驻仅消耗标称算力的 **65.0%**；辅码流（通道 2，纯视频 RTSP）仅在客户端点播时按需拉起，并设为 **`960×720@25fps`**（三路并发总负载 **92.6%**，100% 合规且留有余量）或 **`1440×1080@14fps`**（三路宏块吞吐恰好 **100.0%** 满载合规；若设 `15fps` 则原始宏块达 `102.5%` 临界值，亦可配为双路对称 `1440×1080@19~20fps`）[[15]](#ref-15)。
   - **375 MHz 超频极限压测模式（供样机摸底决策）**：在 U-Boot 引导阶段静态将 `AHB0/VPU` 时钟由 `300 MHz`（`MPLL/5`）超频 **+25%** 至 **`375 MHz`**（`MPLL/4`），使理论吞吐提升至 `306,000 宏块/秒`，可在评估样机上实测“双路 `1440×1080@24~25fps + 1fps`”与“单路 `1440×1080@50fps`”在 `+70°C` 高温长时运行下的稳定性与温升表现，供产品决策是否采纳 [[15]](#ref-15)。

3. **UVC 与模拟输出替代方案**
   - **UVC 高带宽批量传输复合方案**：修复内核 `dwc2` DMA 与 `f_uvc` 驱动，启用 **USB 2.0 HS-Bulk（批量传输，有效带宽 `38~42 MB/s`，硬件重传零丢包）+ `CDC-NCM` 虚拟网卡** Windows 10/11 免驱复合方案，单口同时支持：`640×512@50fps YUY2`（`32.8 MB/s` 原始热像流）、**`1440×1080@25fps MJPEG`**（硬件编码原生内嵌标准哈夫曼表 [[26]](#ref-26)，由上位机实时硬解还原为高清 RGB 画面，替代原生 RGB24）及 **`1440×1080@25fps` Frame-Based `H.264`** [[19]](#ref-19)[[20]](#ref-20)。
   - **模拟 CVBS 处置**：启动阶段软件拉低 `PC07` 彻底关断连接板上的 `MS1836S` 供电（按器件手册 `3.3V+1.2V` 典型工作电流测算可减少 **`> 350 mA`**、典型约 **`428 mA / 1.1 W`** 无效耗电）[[27]](#ref-27)[[28]](#ref-28)；若产品定义必须保留模拟 CVBS 接口，需在下一版硬件改版时新增并口 RGB 或 MIPI 转 CVBS 编码芯片。

---

### 三、其他未在需求中显式暴露、但存在较高风险的事项

除上述规格调整外，源码与原理图深挖还揭示了 5 项未在评估需求文档中显式体现、但直接决定系统能否稳定跑通的高风险工程事项：

1. **VI / VPP 硬件旁路、DPU 回写带宽瓶颈与双核 CPU SIMD 软处理风险**
   - **硬件旁路与回写带宽制约**：X2000 内置的 Tiziano ISP 仅支持 RAW Bayer 传感器，**硬件拒收红外机芯 DVP 输入的 `UYVY`（YUV422）格式** [[29]](#ref-29)，因此采集必须走 `VIC0` ISP-Bypass 直通模式 [[30]](#ref-30)[[31]](#ref-31)（由硬件直接将 `UYVY` 转为 `NV12` 落盘内存，并兼容左 AD + 右 YUV 双半帧模式 [[17]](#ref-17)），无法调用 ISP 后端硬件缩放器。同时，DPU 显示控制器的硬件合成回写（`SRDMA`）**不支持直接输出 `NV12`** [[32]](#ref-32)：虽然实贴 `X2000H`（512 MB）具备充裕的 **`64 MiB`** 多媒体连续内存池（`rmem`），足以容纳全屏双通道 32bpp 合成回写所需的 **`45.96 MiB`** 连续缓冲（含 `17.80 MiB` 三帧回写池，因此 DPU 硬件回写 `Arch R2` 在内存容量上完全可行，可保留作为样机基准对比与备选回退模式）[[33]](#ref-33)，但全屏 `1440×1080@50fps` 32bpp 合成回写**仅 DPU 单模块仍将吞噬 `959 MB/s` 内存总线带宽**，导致整机全负载总带宽攀升至 **`1.89 ~ 1.96 GB/s`**，紧贴 16-bit LPDDR3 多主设备并发可持续上限（`~1.65 ~ 1.95 GB/s`）[[34]](#ref-34)。
   - **软处理主路径与跨核 Cache 风险**：为留足全温域内存带宽裕量，推荐主路径（`Arch R1`）采用 DPU 三层硬件直通上屏（显示带宽仅 **`164 MB/s`**），而编码侧所需的 `640×512 → 1440×1080` 缩放、居中置顶 `3.0×` 画中画拼合、稀疏 UI 叠加与色彩转换，改由 **双核 CPU 的 128-bit `MSA` SIMD 向量指令**承担（将整机全负载总带宽稳控在 **`959 ~ 1,075 MB/s`**；源码核查发现原厂 `libmedia` 自带的 MSA 转换库存在奇偶行重复、GCC 9.3 无法编译及非 16 对齐越界等严重缺陷，必须重写）[[34]](#ref-34)[[35]](#ref-35)。此外，X2000 的 XBurst2 双核 **无硬件跨核 L1 数据缓存一致性**（内核刷缓存指令仅作用于当前核）[[36]](#ref-36)，双核分片并行处理时若未严格执行逐核本地缓存同步，极易引发跨核画面横纹撕裂或关键帧头部损坏，多线程调优与一致性维护风险较高。

2. **16-bit LPDDR3 内存带宽天花板与 UI 框架选型风险（评估期建议轻量引擎/LVGL 替代原版 Qt5）**
   - **512 MB 容量充裕但 16-bit 总线带宽仍是瓶颈**：虽然核心板原理图 `A-PCB-MB-TSIN01` 器件丝印标注为 `X2000` [[37]](#ref-37)，但样机实际贴装芯片为片内集成 **`512 MB`（`512 MiB`）SiP LPDDR3** 的 **`X2000H`** 型号 [[38]](#ref-38)[[39]](#ref-39)（物理地址划分为低端 `256 MiB @ 0x00000000` 与高端 `256 MiB @ 0x30000000` 两段映射 [[40]](#ref-40)[[41]](#ref-41)）。在低 `256 MiB` DMA 可达域分配 `64 MiB` 多媒体连续内存池（`rmem`）、`23.75 MiB` VPU 硬件预留池（`reserved_mem`）及 `256 KiB`（`0.25 MiB`）崩溃黑匣子（`ramoops`）后，Linux 内核与应用程序仍享有充裕的 **`424 MiB` 系统内存**，**彻底消除了内存容量不足（OOM）风险** [[33]](#ref-33)。但需特别注意：即便是 `512 MB` 的 `X2000H`，其片内 DRAM 物理接口位宽依然是 **16-bit LPDDR3-1500** [[39]](#ref-39)[[42]](#ref-42)[[43]](#ref-43)（理论峰值带宽 `3.0 GB/s`，多主设备并发仲裁下可持续总带宽天花板约 **`1.65 ~ 1.95 GB/s`** [[34]](#ref-34)），因此**系统的真正硬瓶颈在于内存总线带宽（`MB/s`）而非内存容量**。
   - **原版 Qt5 的撕裂与高带宽争抢隐患**：源码核查证实，SDK 自带的 Qt 5.12.7 `linuxfb` 插件（`QLinuxFbScreen`）不支持 `FBIOPAN_DISPLAY` 双缓冲硬件翻页（固定写 `yoffset=0`，50Hz 下必然产生视觉撕裂），且现有 `mini-lite-ui` 在全屏透明控件上移动大尺寸十字分划时会触发全屏软件三缓冲拷贝，50fps 下仅 UI 单项即消耗 **`626 ~ 1,866 MB/s`**（`1440×1080` 画布）或 **`278 ~ 830 MB/s`**（`960×720` 画布）内存带宽，与视频采集/编码激烈争抢 16-bit LPDDR3 总线；同时当前开发宿主机工具链亦未预置 Qt5 交叉编译工具（`qmake`/`moc`/`uic`），需通过 Buildroot 全量源码构建并剥离原海思平台路径耦合 [[25]](#ref-25)[[44]](#ref-44)[[45]](#ref-45)。
   - **评估期选型建议与后续 Qt5 兼容路径**：为确保 SoC 全负载压测不受软件 UI 冗余拷贝干扰，评估阶段推荐采用 **双缓冲脏矩形轻量合成引擎**（直接复用现有 `mini-lite-ui` 的 90 张 `496×496` 分划板 PNG 资源 [[45]](#ref-45)，内存占用 `< 3 ~ 4 MiB`，50fps 带宽仅 **`20 ~ 45 MB/s`**，压测峰值 `< 140 MB/s`，确保整机全负载总带宽稳控在 `959 ~ 1,075 MB/s` [[25]](#ref-25)[[34]](#ref-34)）或 SDK 内置的 **LVGL v8.3.9 + MSA** 方案（内存 `4 ~ 6 MiB`，优化后 50fps 带宽约 `110 MB/s` [[25]](#ref-25)[[46]](#ref-46)）。**若后续产品团队坚持保留现有 Qt5 `mini-lite-ui` 框架，依托 `X2000H` 充裕的 `512 MB` 内存空间（`424 MiB` 可用系统内存完全可容纳 Qt5 `35 ~ 55 MiB` 运行开销），无需改动主芯片 BOM 即可承载，前提是软件侧须为 `QLinuxFbScreen` 补齐 `FBIOPAN_DISPLAY` 双缓冲翻页补丁，并将全屏重绘控件改造为局部脏区域刷新以严控内存带宽** [[25]](#ref-25)。

3. **核心板与共用 `JK` 接口板硬件电平失配风险（样机首次上电前必须飞线！）**
   - 经逐页比对核心板与 4 块共用 `JK` 子板原理图（同时勘误了《X2000-MINI 软件沟通表》中 15 处错误 BGA 球号——含 8 处封装不存在球号——及 5 类引脚功能/电平错标 [[47]](#ref-47)[[48]](#ref-48)），发现 **4 项存在击穿主控/外设风险的电气过压隐患**及 **1 组接口/附属电路问题** [[49]](#ref-49)，**样机通电前必须由硬件完成飞线确认**（需特别注意：`512 MB` 的 `X2000H` 同样采用 **`1.20 V` SiP LPDDR3**，`VDDMEM` 绝对耐压上限仍为 **`1.32 V`** [[50]](#ref-50)，隐患 ① 绝不可因容量变化而忽略！）：

   | 风险编号 | 涉及板卡与网络 | 电气失配实情与耐压红线 | 样机上电前必做飞线措施（量产纳入 Rev B 改版） |
   | :---: | :--- | :--- | :--- |
   | **隐患 ①** | 连接板 `CNT_VA` `U11` [[27]](#ref-27) → 核心板 `VCC_1V2` [[37]](#ref-37) | 原供海思的 **`1.35 V`** 直灌 `X2000H` **`1.20 V` LPDDR3**（绝对耐压上限 **`1.32 V`**！）[[50]](#ref-50) | **插接核心板前**，将连接板 `R66` 由 `30 kΩ` 换为 **`24.0 kΩ 1%`**，上电实测确认降至 `1.20 V` [[49]](#ref-49) |
   | **隐患 ②** | 连接板 `PWR_SEQ0` [[27]](#ref-27) → 核心板 `F3 (RTC_PWRON)` [[51]](#ref-51) | 开机键按下时 **`4.27 V`**（稳态约 `3.27 V`）经 `0Ω` 直灌 `1.8 V` RTC 引脚（耐压上限 **`1.98 V`**！）[[52]](#ref-52) | 拆除核心板 `R170 (0Ω)`，加装共栅极 N-MOS 电平钳位管（栅极接 `1.80 V` `VDDIORTC`）[[49]](#ref-49) |
   | **隐患 ③** | 核心板 `I2C5` (`PC27/28`) → 连接板 `ICM-20948` [[53]](#ref-53)[[21]](#ref-21) | 电平转换管 `Q6/Q7` 未贴、`0Ω` 直通，**`3.3 V`** 直灌 **`1.8 V` IMU**（工作上限 `1.95 V`、引脚耐压 `2.30 V`）[[54]](#ref-54) | 拆除连接板 `R71/R72 (0Ω)`，补焊 `Q6/Q7` 双 MOS 管及两侧 `4.7 kΩ` 上拉电阻 [[49]](#ref-49) |
   | **隐患 ④** | 核心板 `VDDIO_SD` (`Q1` 3.3V / `U2` 1.8V) [[37]](#ref-37) | `R8 (10kΩ)` 默认下拉使上电导通 `Q1` 输出 **`3.30 V`** 直灌后级 1.8V WiFi [[55]](#ref-55) / IMU [[21]](#ref-21)；且因 `PC00` 内部下拉电阻（最小 `27 kΩ`、典型 `48 kΩ`、最大 `103 kΩ` [[56]](#ref-56)）分压，仅将 `R8` 改上拉而不拆 `Q1` 仍会导致复位期 **`3.3V` 与 `1.8V` 同时导通倒灌** | 样机必须**同时拆除核心板 3.3V P-MOS `Q1` 并将 `R8` 改接 `3.3 V` 上拉**，从上电零时刻起物理锁定纯 `1.80 V` 供电 [[49]](#ref-49) |
   | **接口与附属** | 核心板 `CON7` [[23]](#ref-23) ↔ OLED 板 `CON2` [[57]](#ref-57) 等 | 核心板 **21-pin**（2-Lane）与 OLED 板 **45-pin**（4-Lane）座子不匹配 [[23]](#ref-23)[[57]](#ref-57)；连接板纽扣电池 `BT1` [[27]](#ref-27) 经核心板 `D7` 向关断态 `VCC_5V` 倒灌漏电 [[58]](#ref-58)；调试串口 TX/RX 反接 [[23]](#ref-23)；OLED 复位脚上下拉冲突 [[23]](#ref-23)[[57]](#ref-57)（及测距串口 `U10` 电平失配 [[53]](#ref-53)） | 定制 100Ω 阻抗受控 **21pin↔45pin OLED 转接 FPC**；**割断核心板 `VCC_5V` 至 `U8.IN`（`D7` 阴极节点）铜皮并在 `VCC_5V` 侧串入防倒灌肖特基管**（组成双管或门）；外部调试线交叉 TX/RX；**拆除核心板 OLED 复位脚冲突上拉 `R148`**（启用测距 `UART9` 前亦需飞线旁路连接板 `U10`）[[49]](#ref-49) |

4. **低温工作下限与片上测温盲区风险**
   - **外围器件低温指标制约**：`X2000H` 主控虽支持 `-40°C ~ +85°C` [[59]](#ref-59)，但当前核心板选用的 FORESEE 64GB eMMC（`FEMDNN064G-C9A61`，原理图兼容丝印 `FEMDRW064G-88A19` [[60]](#ref-60)）标称工作温度为 **`-25°C ~ +85°C`** [[61]](#ref-61)，而 `SY049WDM01` Micro-OLED 标称工作温度仅为 **`-20°C ~ +70°C`**（`-40°C` 仅为不工作存储温度）[[62]](#ref-62)。若整机定型要求满足 **`-30°C` 或 `-40°C`** 严寒冷启动与工作，现有 eMMC 需替换为同封装 `-40°C` 宽温工业级料号，OLED 屏亦需评估加热或低温降额策略 [[49]](#ref-49)。
   - **主控无内置温度传感器**：X2000 芯片内部**未集成 CPU 温度传感器**（片上 `SADC` 仅测外部模拟电压）[[63]](#ref-63)[[64]](#ref-64)。软件侧将联合轮询 USB 子板充电芯片 `BQ25798` [[65]](#ref-65)[[66]](#ref-66)、连接板陀螺仪 `ICM-20948` [[67]](#ref-67) 及 OLED 屏内部温度寄存器 [[8]](#ref-8) 构建板级测温闭环 [[68]](#ref-68)，但高低温摸底测试时需在主控表面贴附热电偶辅助标定温升。

5. **原厂 SDK 底层缺陷修复与跨架构（ARM → MIPS32）兼容周期风险**
   - 代码审计在君正原厂 Linux 5.10 内核与驱动中定位了 **20 余处阻断性底层缺陷**——涵盖 **Helix VPU 8 项缺陷**（默认用户态码控守护进程因 `GLIBC_2.29` 库版本不匹配直接失效须切回内核态码控 [[69]](#ref-69)、`USERPTR` 漏加页内偏移导致 1440P 色度错位 [[70]](#ref-70)、JPEG 按 1088 行越界读取 [[71]](#ref-71)、码率上限锁死 `4 Mbps` 与帧率硬编码 `25 fps` 及缓冲校验写反 [[72]](#ref-72)、以及跨核 `SPS/PPS` 缓存失步 [[73]](#ref-73) 等 [[15]](#ref-15)）、**DPU/DSI 6 项缺陷**（像素时钟分频向上取整导致行频暴跌 `-12.5%` [[74]](#ref-74) 引发屏幕保护关断、`SLPOUT` 唤醒期无行同步脉冲 [[75]](#ref-75) 及零帧/翻页与描述符缓存刷新异常等 [[9]](#ref-9)[[76]](#ref-76)[[77]](#ref-77)）、`VIC0` 采集持锁等待死锁 [[30]](#ref-30)、USB `dwc2` 散聚 DMA 空指针崩溃 [[20]](#ref-20)[[78]](#ref-78) 及看门狗 [[79]](#ref-79) 与硬件 DDR 带宽监控驱动异常 [[68]](#ref-68)[[80]](#ref-80)。
   - 同时，既有海思（ARM 架构）`mini-lite` 仓库中的闭源第三方库（如 `librtsp.so` 等）无法在 **MIPS32 小端序**架构 [[13]](#ref-13) 上运行，需切换至开源 `xop` RTSP 服务并修复 7 处跨架构符号性与并发缺陷 [[81]](#ref-81)[[82]](#ref-82)。相关底层排雷与联调工作量客观存在，需在项目排期中预留合理周期。

---

### 四、已确认风险可控 / 无风险的内容

除上述需重点管控的边界与风险外，以下核心模块经核查方案成熟、海思 `master_mini` 近期量产加固成果可直接平移，整体风险可控：

1. **`X2000H` 512 MB 充裕内存空间与多池物理隔离布局**：实贴 `X2000H` 片内集成 **`512 MiB` SiP LPDDR3** [[38]](#ref-38)[[39]](#ref-39)，在低 `256 MiB` DMA 可达域分配充裕的 **`64 MiB` 多媒体连续内存池（`rmem`）**、**`23.75 MiB` VPU 专用池（`reserved_mem`）** 与 **`256 KiB`（`0.25 MiB`）`ramoops` 黑匣子** 后，Linux 内核与应用程序仍享有 **`424 MiB` 系统可用内存**（`168 MiB` 低端内存 + `256 MiB` 高端内存）[[33]](#ref-33)[[41]](#ref-41)，不仅可轻松承载 **`12 MiB` 5 秒高清 GOP 预录环形缓存** 与长时并发队列，也为后续叠加测距、弹道解算等扩展业务预留了充足余量，**无需更改主控芯片 BOM**。
2. **64 GB eMMC 高速适配、只读系统分区与防掉电损坏架构**：X2000 `MSC0` 原生支持 8-bit `1.8V` **HS200 模式** [[18]](#ref-18)[[83]](#ref-83)（运行于合规 `93.75 MHz ≤ 104 MHz`，预估可持续顺序吞吐约 `85 MB/s`，远高于录制写入带宽需求 [[84]](#ref-84)[[85]](#ref-85)）；统一采用 8 MiB 对齐的 10 分区 GPT 布局（基于可用用户区 `59,680 MiB` [[86]](#ref-86)），将 `rootfs`（A/B 双备份 SquashFS，配合独立裸分区存放 `kernel`）、`/app` 与 `/manufacturer` 全部设为**只读挂载**，将用户配置 `/config` 与 **`57.46 GiB` 大容量录像分区 `/userdata`** 物理隔离，并配合内核脏页字节限流（`2 MiB / 6 MiB`），从根源上杜绝异常断电导致系统损坏或脏页堆积 [[85]](#ref-85)[[87]](#ref-87)。
3. **5 秒预录环形缓存（`12 MiB`）、30 秒周期切片与长时持续 MP4 录制**：依托 512 MB 充裕内存采用零动态分配的 **`12 MiB` 内存 GOP 环形缓冲（稳定覆盖最新 5 秒高清码流并留足码率裕量）** [[33]](#ref-33)[[82]](#ref-82)，配合纯离线轻量化 **分片式 `fMP4` 封装器**（每 2 秒写入独立 `moof+mdat` 片段并按 `1 MiB` 聚合刷盘，规避 eMMC 内部 `64 MiB` 易失性缓存 [[83]](#ref-83) 断电丢索引问题）[[82]](#ref-82)，可稳定支撑长时持续录像、每 30 秒自动截取“前 5 秒预录 + 后 10 秒实时 = 15 秒独立 `.mp4`”、`1fps` `.jpg` 抓拍及 `85%→75%` 磁盘水位自动循环覆盖，任意时刻拔电最多仅损失末尾 `≤ 2 秒` 视频 [[82]](#ref-82)。
4. **共用子板外设、5 GHz WiFi / RTSP 点播与系统级可靠性**：海思 `master_mini`（PR#1~PR#21）已验证成熟的 **`BQ25798` 双 2S 电池充放电管理与 4 层 5% 步进电量算法** [[88]](#ref-88)（修正内外电池 SADC 通道对调后直接复用 [[89]](#ref-89)）、**`ICM-20948` 九轴 IMU（含内置磁力计与空闲零中断门控）** [[90]](#ref-90)、**5 GHz `AIC8800D80`（`BL-M8800DS2` [[55]](#ref-55)）SDIO WiFi 与多路 RTSP 实时点播推流** [[82]](#ref-82)[[89]](#ref-89)、**四按键组合抑制与“10s 同步关机 + 3s 硬件强断”防死机保护** [[87]](#ref-87)[[89]](#ref-89)、以及 **双心跳硬件看门狗 + `256 KiB` `ramoops` 崩溃黑匣子日志归档** [[33]](#ref-33)[[87]](#ref-87)，在完成上电前电平飞线修正后均可平滑落地。

---

### 五、下一步推进建议与待决策事项

为高效推进样机点亮与 72 小时全负载压测，恳请各位领导审阅并协助确认以下事项：

1. **显示与分划方案确认**：确认评估阶段采用 **`960×720@50Hz`（屏端 `1.5×` 硬件放大）+ 录像原生 `1440×1080` 高清分划** [[9]](#ref-9) 方案推进，并在样机点亮后组织实测评审目镜端十字分划主观锐度（同步对比 `1080×810` `1.33×` 档位）。
2. **多路编码与评估期 UI 策略确认**：确认以 **300 MHz 稳态合规模式**（主路 `1440×1080@25fps` 常驻 + 辅路按需/`960×720@25fps` 或 `1440×1080@14fps` + `1fps` 抓拍）[[15]](#ref-15) 搭配 **双缓冲脏矩形轻量合成引擎 / LVGL** [[25]](#ref-25) 作为全负载压测基准目标，同时在样机上补充 **375 MHz 超频模式** 高温稳定性摸底数据；若量产阶段确定沿用现有 Qt5 `mini-lite-ui`，在 `X2000H` `512 MB` 内存基础上同步排期完成 `QLinuxFbScreen` 双缓冲硬件翻页与局部脏区域刷新改造 [[25]](#ref-25)。
3. **硬件首板飞线、Rev B 固化与低温选型确认**：请硬件同事协助在首台 `MB-TSIN01` 样机上电前完成上述 **4 项过压电平隐患飞线**（实贴 `512 MB` `X2000H` 仍为 `1.20 V` LPDDR3，`CNT_VA` `R66` 必换 `24.0 kΩ` [[49]](#ref-49)[[50]](#ref-50)）、防倒灌/复位冲突飞线及 **21pin↔45pin OLED 转接排线** 制作 [[49]](#ref-49)；因样机已实贴 `512 MB` `X2000H`，**无需升级主芯片内存型号**，下一版硬件（Rev B）重点将首板电平与接口勘误固化入板（及按需评估是否新增 CVBS 桥接芯片），并结合整机定型低温指标确认是否替换 **`-40°C` 宽温工业级 eMMC** [[61]](#ref-61) 及制定 **OLED 屏低温加热/降额策略** [[62]](#ref-62)。

以上风险评估如有不周之处，敬请各位领导批评指正；待方向确认后我们将按模块并行推进底层驱动移植与全负载仿真压测开发。

顺祝商祺！

**肖劲涛**  
`<jintao.xiao@vitalchem.com>`

---

### 参考资料与核查依据

- <a id="ref-1"></a>**[1]** 《x2000评估验证，仿真跑全负载场景》，`ingenic/docs/target/x2000评估验证，仿真跑全负载场景.md` — **一~五（L3–L51）**：定义 VI/VO 媒体通路、动态枪瞄十字分划 UI、三通道编码（`2×25fps + 1fps`）、RTSP/MP4/UVC 录制推流及高低温长时全负载压测指标。
- <a id="ref-2"></a>**[2]** Ingenic，《X2000 AIoT Application Processor Data Sheet》（2022-06-21），`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_DS_v1.40.pdf` — **§1.2.2 (p.6) & §1.2.9 (p.9)**：标称最高 `1920×1080@40Hz` 显示输出与 H.264/JPEG `1920×1080@30fps` 硬件编码规格（无 H.265）。
- <a id="ref-3"></a>**[3]** Ingenic，《X2000 Data Sheet》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_DS_v1.40.pdf` — **Fig. 1-1 (p.5) & Table 2-1 (p.26–27)**：系统框图与引脚定义仅封装 2 对 MIPI-DSI 数据通道（芯片引脚名 `TX_DATAP0/N0`、`TX_DATAP1/N1`），无 VESA-DSC 压缩硬件且无原生 HDMI TX 引脚。
- <a id="ref-4"></a>**[4]** Ingenic，《X2000 IoT Application Processor Programming Manual》（2023-07-11），`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_PM_20230711.pdf` — **Ch. 11 & Ch. 12（正文 p.285, p.307, p.324 / PDF p.319, p.341, p.358）**：MIPI-DSI 控制器与 D-PHY 发送端硬件最多支持 2 条数据通道（2 Data Lanes）。
- <a id="ref-5"></a>**[5]** 君正 X2000 MIPI-DSI 控制器驱动源码，`ingenic/vendor/module_driver/soc/x2000_510/mipi_dsi/jz_mipi_dsi.c` — **L147**：底层驱动硬性校验最大通道数为 2-Lane（`num_of_lanes > 2` 报错），无 DSC 压缩配置。
- <a id="ref-6"></a>**[6]** SeeYA Technology（视涯科技），《SeeYA 0.49" Micro-OLED Specification — Model: SY049WDM01》（`SY-SPEC-045P` V1.3，2023-11-28），`ingenic/docs/board/SY-SPEC-045P_SEEYA_SY049WDM01_Specification_V1.3_20231128.pdf` — **§1 (p.5)、§8.3.1 (p.16) & §8.4.1 (p.18)**：`§1 p.5` 明确 MIPI 单通道速率上限 `1.0 Gbps/Lane`，`§8.3.1 p.16` 定义 HS 接收端（Receiver）电气特性，`§8.4.1 p.18` 规定 `UIINST = 1~3 ns` 即 `UI ≥ 1.0 ns`（另见 `§8.3.2 Note 1 p.17` 标注 `≤ 1 Gbps`）。
- <a id="ref-7"></a>**[7]** SeeYA，《SY049WDM01 Specification》，`ingenic/docs/board/SY-SPEC-045P_SEEYA_SY049WDM01_Specification_V1.3_20231128.pdf` — **§1 (p.5) & §11 `CMODE 0300h` / `COLMOD 3A00h` (p.27, p.29, p.44–45)**：面板虽内置 DSC 解码器，但在未压缩模式下仅支持 24-bit `RGB888` 输入（不支持 `RGB565` 等低位宽格式）。
- <a id="ref-8"></a>**[8]** 海思量产基线 Micro-OLED 驱动，`Hi3559V200_SDK_V2.0.1.0/open_source/linux/linux-4.19.y/drivers/video/sy049_dsi/sy049_dsi.c` — **`master_mini` PR#10 / PR#11 / PR#20**：实测确立 `SY049WDM01` 内部电荷泵 `f_H ≈ 74.88~74.90 kHz` 锁行法则（偏离则 1.5s 白闪、30s 关断）、`SLPOUT` 后 `1600 ms` 禁切页保护及 `P12:BF02` 屏温寄存器读取。
- <a id="ref-9"></a>**[9]** 《Ingenic X2000 Mini-Lite Porting & Full-Load Verification Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **§0.1.1–§0.1.2 & `TODO-05` §1–§4（L23–L35, L1753–L1921）**：2-Lane MIPI-DSI 原生 `1440×1080@50Hz` 超限推演、`960×720@50Hz`（`1.5×` 缩放）/ `1280×720` / `1080×810`（`1.33×`）合规时序、`1440×1080` 双尺度零光轴偏移渲染及 DPU/DSI 6 项底层修复（含 `VPG_EN` 唤醒保活）。
- <a id="ref-10"></a>**[10]** Ingenic，《X2000 Programming Manual》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_PM_20230711.pdf` — **Ch. 4《H264 Jpeg Encoder/Decoder (Helix)》（正文 p.23–36 / PDF p.57–70）**：`Helix` 硬件视频编码单元仅支持 H.264 编码与 JPEG 编解码，无 H.265 逻辑。
- <a id="ref-11"></a>**[11]** Ingenic，《X2000 Programming Manual》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_PM_20230711.pdf` — **Ch. 3《H264 Decoder (Felix)》（正文 p.14–22 / PDF p.48–56）**：独立硬件解码单元 `Felix` 仅支持 H.264 解码。
- <a id="ref-12"></a>**[12]** 君正内核 Helix VPU 驱动源码，`ingenic/vendor/kernel/kernel/module_drivers/drivers/media/platform/ingenic-vcodec/helix/helix_drv.c` — **L337–L353, L474**：仅支持 H.264/JPEG 编码，且多实例通过单一有序工作队列（`encode_workqueue`）逐帧串行分时复用硬件引擎。
- <a id="ref-13"></a>**[13]** Ingenic，《X2000 Data Sheet》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_DS_v1.40.pdf` — **§1.2.1 (p.6)**：双核 XBurst2 @ 1.2 GHz（MIPS32r5 小端序架构）与 128-bit MSA SIMD 向量引擎规格。
- <a id="ref-14"></a>**[14]** 君正内核 X2000 时钟驱动源码，`ingenic/vendor/kernel/kernel/module_drivers/drivers/clk/ingenic-v2/clk-x2000.c` — **L114, L182**：`Helix` VPU 挂载于 `div_ahb0` 总线时钟（标称频率 `300 MHz`，即 `MPLL 1500 MHz / 5`）。
- <a id="ref-15"></a>**[15]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **`TODO-07` §7.1–§7.3（L2129–L2258）**：`Helix` 300 MHz 宏块吞吐核算、8 项内核编码缺陷（`V1..V8`）修复及 300 MHz 稳态合规模式 vs 375 MHz 超频压测模式设计。
- <a id="ref-16"></a>**[16]** 《x2000评估验证，仿真跑全负载场景》，`ingenic/docs/target/x2000评估验证，仿真跑全负载场景.md` — **前期摸底记录与备选全负载场景（L55–L72）**：前期 `1280×1024` 超过 40fps 出现 UV 颜色异常现象记录，以及单路全分辨率 `50fps` 编码备选压测指标。
- <a id="ref-17"></a>**[17]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **`TODO-06` §3–§4（L2075–L2091）**：左 AD + 右 YUV 双半帧采集兼容方案，及 `1280×1024 >40fps` UV 色度异常四重根因复盘。
- <a id="ref-18"></a>**[18]** Ingenic，《X2000 Data Sheet》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_DS_v1.40.pdf` — **§1.2.11 (p.12)**：`MSC0` 8-bit eMMC 5.1（最高时钟 `104 MHz`）与 USB 2.0 OTG High-Speed（480 Mbps）接口规格。
- <a id="ref-19"></a>**[19]** 君正内核 UVC Gadget 驱动源码，`ingenic/vendor/kernel/kernel/drivers/usb/gadget/function/f_uvc.c`：原厂默认同步传输限制及补齐 HS-Bulk 批量传输、Frame-Based H.264 与 Windows ZLP 补丁依据。
- <a id="ref-20"></a>**[20]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **`TODO-08` §1–§4（L2888–L3081）**：USB 2.0 HS-Bulk（`38~42 MB/s`）与同步传输（`24.58 MB/s`）线速核算、`CDC-NCM` 免驱复合枚举及 `YUY2 / MJPEG / H.264` 三格式方案。
- <a id="ref-21"></a>**[21]** 共用连接板硬件原理图，`ingenic/docs/board/A-PCB-JK-THG-CNT_VA.pdf`（`VA`，2026-09-14）— **p.5**：`U7 MS1836S` HDMI 转 CVBS 电路与 `U8 ICM-20948`（`VDD`/`VDDIO` 均接 `1.8V`）外围电路。
- <a id="ref-22"></a>**[22]** 宏晶微电子，《MS1836S HD 转 CVBS/S-Video 转换器数据手册》（`HJ-BMJL-PD-002` V1.4，2023-11-23），`ingenic/docs/board/ms1836s.pdf` — **§1–§2 (p.2)、§7 Table 7.1 (p.7) & §8.4 Table 8.5 (p.10)**：视频输入端仅支持 HDMI/TMDS 差分信号。
- <a id="ref-23"></a>**[23]** 核心板硬件原理图，`ingenic/docs/board/A-PCB-MB-TSIN01.pdf`（`V1.0`，修订表 `2026-07-21` / 图框 `20261105`）— **p.9**：`CON5` 100-pin B2B 座（对应位号实为 3.3V 并口 `LCD_3V3_*` 信号，且调试串口网络 `DEBUG_UART_TXD/RXD` 与 `UART3_3V3_RXD/TXD` 交叉反接）、`CON7` 21-pin 2-Lane MIPI-DSI 座及 `R148`（`4.7 kΩ`）OLED 复位上拉电阻。
- <a id="ref-24"></a>**[24]** SeeYA，《SY049WDM01 Specification》，`ingenic/docs/board/SY-SPEC-045P_SEEYA_SY049WDM01_Specification_V1.3_20231128.pdf` — **§1–§2 (p.5–6) & §10.2.1 / §11 `SCACTRL 6900h`、`IFCONF 6B00h` (p.26–27, p.47–48)**：内置 `1.33× / 1.5× / 2×` 硬件放大器、2-Lane 接口配置、面板 `90 Hz` 最高扫描帧率及 `1280×720 → 1920×1080` 硬件缩放示例。
- <a id="ref-25"></a>**[25]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **`TODO-09` §9.1–§9.4（L3117–L3214）**：`1440×1080` 统一逻辑光轴双尺度脏矩形轻量合成引擎、LVGL v8.3.9 与 Qt 5.12.7 `linuxfb` 内存带宽对比（`20~45 MB/s` vs `626~1,866 MB/s` @ 1440p50 / `278~830 MB/s` @ 720p50）及 Qt 双缓冲翻页改造方案。
- <a id="ref-26"></a>**[26]** 君正内核 Helix JPEG 编码算子源码，`ingenic/vendor/kernel/kernel/module_drivers/drivers/media/platform/ingenic-vcodec/helix/jpge.c` — **L149–L174**：`jpge_gen_header()` 原生内嵌 4 张标准 `DHT (0xFFC4)` 哈夫曼表，兼容 Windows 10/11 UVC 免驱硬解。
- <a id="ref-27"></a>**[27]** 共用连接板硬件原理图，`ingenic/docs/board/A-PCB-JK-THG-CNT_VA.pdf` — **p.3**：受 `MS1836_EN` 控制的 `U17/U12` 供电开关、`U11 SY8823QUC`（`R66=30 kΩ / R69=24 kΩ` 输出 `1.35 V`）、`U6 TMI3258`（`4.27 V` 主电源）及 `BT1`（`MS412FE-FL26`）纽扣电池电路。
- <a id="ref-28"></a>**[28]** 宏晶微电子，《MS1836S 数据手册》，`ingenic/docs/board/ms1836s.pdf` — **§8.2 Table 8.3 (p.9)**：`3.3 V + 1.2 V` 各供电轨典型工作电流合计 `428 mA`（折合典型功耗约 `1.1 W`）。
- <a id="ref-29"></a>**[29]** 君正 X2000 Tiziano ISP 通道驱动源码，`ingenic/vendor/module_driver/soc/x2000_510/camera/isp/vic_channel_tiziano.c` — **L326–L339**：Tiziano ISP 输入格式表仅接收 RAW Bayer 传感器格式，硬件拒收 DVP `UYVY`（YUV422）数据。
- <a id="ref-30"></a>**[30]** 君正 X2000 `VIC0` 直通采集驱动源码，`ingenic/vendor/module_driver/soc/x2000_510/camera/vic/vic_channel_mem.c` — **L334, L645–L679**：`VIC0` ISP-Bypass 硬件 `UYVY→NV12` 直通 DMA 采集机制，以及 `camera_get_frame()` 跨等待队列持互斥锁死锁缺陷。
- <a id="ref-31"></a>**[31]** Ingenic，《X2000 Programming Manual》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_PM_20230711.pdf` — **Ch. 6《Video Interface Control (VIC)》（正文 p.125–168 / PDF p.159–202）**：`VIC` 视频接口直通采集与硬件 `UYVY→NV12` DMA 格式转换规范。
- <a id="ref-32"></a>**[32]** Ingenic，《X2000 Programming Manual》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_PM_20230711.pdf` — **Ch. 10 §10.4.1.1（正文 p.242 / PDF p.276）**：DPU 显示控制器合成回写格式 `WB_Format` 仅支持 `ARGB8888`/`RGB888`/`RGB565`/`RGB555`，不支持 `NV12` 直接回写。
- <a id="ref-33"></a>**[33]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **§2.3（L321–L341）& §10.2（L3379–L3388）**：`X2000H` `512 MiB` 物理内存布局（`64 MiB` `rmem` + `23.75 MiB` VPU `reserved_mem` + `0.25 MiB` `ramoops` + `424 MiB` Linux 系统内存，兼容 `45.96 MiB` DPU 回写池与 `12 MiB` 5s GOP 环形缓冲）。
- <a id="ref-34"></a>**[34]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **`TODO-07` §7.4–§7.5（L2259–L2847）**：16-bit LPDDR3-1500 多主设备可持续带宽天花板（`1.65~1.95 GB/s`）、全屏 32bpp 回写 `Arch R2`（DPU `959 MB/s`、整机 `1.89~1.96 GB/s`）vs 直通合成 `Arch R1`（DPU `164 MB/s`、整机 `959~1,075 MB/s`）核算及重写 MSA SIMD 算子方案。
- <a id="ref-35"></a>**[35]** 君正 SDK 多媒体色彩转换源码，`ingenic/vendor/libmedia/core/convert/msa_convert.c` — **L97–L133**：宏参数写死 `p0` 导致奇偶行重复、GCC 7.2 私有内建函数在 GCC 9.3 下无法编译及非 16 对齐宽度越界缺陷。
- <a id="ref-36"></a>**[36]** 君正内核 MIPS 缓存一致性源码，`ingenic/vendor/kernel/kernel/arch/mips/mm/c-r4k.c` — **L891–L896**：XBurst2 双核 `r4k_blast_dcache()` 仅刷新当前执行核本地 L1 数据缓存，无硬件跨核 SMP 缓存一致性广播。
- <a id="ref-37"></a>**[37]** 核心板硬件原理图，`ingenic/docs/board/A-PCB-MB-TSIN01.pdf` — **p.4**：主控电源符号印为 `U1B X2000`、`VDDMEM_0..8` 连接 `VCC_1V2`，以及 `Q1`（`CJ3139K`）/ `U2`（`TPS7A2018`）/ `R8`（`10 kΩ` 下拉）组成的 `VDDIO_SD` 切换电路。
- <a id="ref-38"></a>**[38]** 君正 U-Boot 芯片型号配置源码，`ingenic/vendor/bootloader/uboot-x2000/tools/ingenic-tools/ddr_creator_x2000/add_chip_info.c` — **L36–L40**：定义 `X2000H` 为片内封装 `512 MiB`（`MEM_512M`）SiP `LPDDR3` 的大容量型号。
- <a id="ref-39"></a>**[39]** 君正 U-Boot `X2000H` 内存颗粒参数头文件，`ingenic/vendor/bootloader/uboot-x2000/include/ddr/chips-v2/LPDDR3_NK6CL256M16DKX-H1.h` — **L23, L32, L126–L133**：`X2000H` 合封 16-bit `512 MiB` LPDDR3 颗粒（`NK6CL256M16DKX-H1`），数据率定义为 `CONFIG_DDR_DATA_RATE = MEM_FREQ × 2`。
- <a id="ref-40"></a>**[40]** Ingenic，《X2000 Programming Manual》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_PM_20230711.pdf` — **Ch. 17 §17.3 Fig. 17-1 & Table 17-2（正文 p.547–548 / PDF p.581–582）**：物理地址空间划分（低端 `0x00000000..0x0FFFFFFF` 256 MB SDRAM、中间 `0x10000000..0x1FFFFFFF` 外设/保留区、高端 `0x20000000..0xDFFFFFFF` 扩展 SDRAM 区）。
- <a id="ref-41"></a>**[41]** 君正 U-Boot SPL 内存参数生成源码，`ingenic/vendor/bootloader/uboot-x2000/common/spl/spl_auto_args_mem.c` — **L70–L89**：对 `512 MiB` 机型自动生成低端 `256 MiB @ 0x00000000`（含 `rmem`）与高端 `256 MiB @ 0x30000000` 两段式内核内存参数。
- <a id="ref-42"></a>**[42]** 君正 U-Boot X2000 公共板级配置头文件，`ingenic/vendor/bootloader/uboot-x2000/include/configs/x2000_base_common.h` — **L19, L86**：配置 `CONFIG_SYS_MEM_FREQ = 750000000`（`750 MHz` 时钟对应 `LPDDR3-1500`、16-bit 下理论峰值带宽 `3.0 GB/s`），且 `CONFIG_DDR_DW32 = 0` 固定为 16-bit 总线位宽。
- <a id="ref-43"></a>**[43]** Ingenic，《X2000 Data Sheet》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_DS_v1.40.pdf` — **§1.2.7 (p.8)**：片上 DDR 控制器物理数据总线位宽固定为 **16-bit** LPDDR3（最高时钟 `800 MHz`）。
- <a id="ref-44"></a>**[44]** 君正 Buildroot Qt 5.12.7 构建配置，`ingenic/vendor/buildroot/buildroot/package/qt5/qt5base/qt5base.mk`：SDK 宿主机工具链未预置 Qt5 交叉编译工具，且原版 `qtbase-5.12.7` `qlinuxfbscreen.cpp`（L367–L420）仅映射 `yoffset=0`、无 `FBIOPAN_DISPLAY` 双缓冲硬件翻页。
- <a id="ref-45"></a>**[45]** 既有 UI 业务仓库 `mini-lite-ui`（`main` 分支 `ac18ba671`，独立外部仓）：`MainGui/funcwidget/ReticleWidget.cpp`（L19–L88，全屏透明控件移动重绘）及 `MainGui/resource/funcwidget/reticle/` 下 90 张 `496×496` 分划板 PNG 资源。
- <a id="ref-46"></a>**[46]** 君正 SDK LVGL v8.3.9 显示适配源码，`ingenic/vendor/third_party/lvgl/ingenic/lvgl_fb_display_v8.39.c`：支持 Framebuffer 双缓冲翻页与 MSA SIMD 混合加速。
- <a id="ref-47"></a>**[47]** 硬件引脚对接沟通表，`ingenic/docs/board/X2000-MINI软件沟通表.xlsx`：对照 BGA270 球位图核查出 15 处错误 BGA 球号（含 8 处封装不存在球号）及 5 类引脚功能/电平错标。
- <a id="ref-48"></a>**[48]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **§2.2.C（L302–L318）**：软件沟通表全部错误 BGA 球号与信号/电平错标逐项勘误表。
- <a id="ref-49"></a>**[49]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **`TODO-00` §00.1–§00.3（L404–L449）**：样机首次上电前必做硬件飞线表（`RF-01`~`RF-16`）、21pin↔45pin OLED 转接 FPC 线序及低温器件风险清单。
- <a id="ref-50"></a>**[50]** Ingenic，《X2000 Data Sheet》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_DS_v1.40.pdf` — **§3.1 Table 3-1 (p.34) & §3.2 Table 3-2 (p.35)（`VDDMEM`）**：LPDDR3 内存供电 `VDDMEM` 推荐典型工作电压 `1.20 V`（`1.14~1.30 V`），绝对最大耐压上限 **`1.32 V`**。
- <a id="ref-51"></a>**[51]** 核心板硬件原理图，`ingenic/docs/board/A-PCB-MB-TSIN01.pdf` — **p.5**：`R170 (0Ω)` 将外部 `PWR_SEQ0` 直连至主控 `1.8 V` RTC 域引脚 `F3 (RTC_PWRON)`。
- <a id="ref-52"></a>**[52]** Ingenic，《X2000 Data Sheet》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_DS_v1.40.pdf` — **§3.1 Table 3-1 (p.34) & §3.2 Table 3-2 (p.35)（`VDDIORTC`）**：RTC 域推荐工作电压 `1.80 V`，`VDDIORTC` 供电与对应引脚绝对最大耐压上限 **`1.98 V`**。
- <a id="ref-53"></a>**[53]** 共用连接板硬件原理图，`ingenic/docs/board/A-PCB-JK-THG-CNT_VA.pdf` — **p.4**：对接核心板 `I2C5` 的电平转换电路（子板图面沿用海思网络名 `I2C7`，`Q6/Q7` 未贴、`R71/R72 = 0Ω` 直通），以及测距串口 `U10 MS4553S`（`VCCA=1.8V`，子板网络名 `UART4`）电平失配电路。
- <a id="ref-54"></a>**[54]** InvenSense / TDK，《ICM-20948 9-Axis MEMS MotionTracking Device Datasheet》（`DS-000189` Rev 1.5，2021-09-02），`ingenic/docs/board/ds-000189-icm-20948-v1.5.pdf` — **§3.4 Table 4 (p.13) & §3.7 Table 9 (p.18)**：`VDDIO` 推荐工作电压 `1.71~1.95 V`，绝对最大耐压 `2.5 V`，数字引脚耐压 `VDD + 0.5 V`（`1.8 V` 供电下上限 `2.30 V`）。
- <a id="ref-55"></a>**[55]** 共用 WiFi/按键子板硬件原理图，`ingenic/docs/board/A-PCB-JK-THG02-WIFI.pdf`（`VA`）— **p.2–p.3**：四按键 `SW1~SW4` 与 `U2 BL-M8800DS2`（`AIC8800D80`，`1.8V` SDIO 及控制电平）5 GHz WiFi 模组电路。
- <a id="ref-56"></a>**[56]** Ingenic，《X2000 Data Sheet》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_DS_v1.40.pdf` — **§3.3 Table 3-9 (p.39)**：`PC00` 等下拉引脚内部下拉电阻 `RPD` 规格（最小 `27 kΩ`、典型 `48 kΩ`、最大 `103 kΩ`）。
- <a id="ref-57"></a>**[57]** 共用 OLED 子板硬件原理图，`ingenic/docs/board/A-PCB-JK-THG02-OLED.pdf`（`V1.0`）— **p.2**：`CON2` 45-pin 4-Lane FPC 座、`R7`（`4.7 kΩ`）OLED 复位下拉电阻与 `U5 TPS65132WRVCR` 偏压电源。
- <a id="ref-58"></a>**[58]** 核心板硬件原理图，`ingenic/docs/board/A-PCB-MB-TSIN01.pdf` — **p.8**：肖特基管 `D7` 阴极直连全局 `VCC_5V` 与 `U8/U9` RTC LDO 输入端，导致关断态纽扣电池 `BT1` 向 `VCC_5V` 倒灌漏电。
- <a id="ref-59"></a>**[59]** Ingenic，《X2000 Data Sheet》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_DS_v1.40.pdf` — **§3.1 Table 3-1 (p.34)（工作温度）**：X2000 系列主控标称工作环境温度范围 `-40°C ~ +85°C`。
- <a id="ref-60"></a>**[60]** 核心板硬件原理图，`ingenic/docs/board/A-PCB-MB-TSIN01.pdf` — **p.7**：板载 8-bit 64GB eMMC `U3` 电路（原理图兼容丝印 `FEMDRW064G-88A19`，`VCCQ=1.8V`）。
- <a id="ref-61"></a>**[61]** Longsys（江波龙），《FORESEE eMMC Datasheet — FEMDNN064G/128G/256G-C9A61》（`E-01132` V0.1，2025-01-03），`ingenic/docs/board/eMMC FEMDNN064G-C9A61 Datasheet Version 0.1(2).pdf` — **Key Features & §2.1 Table 2（PDF p.2, p.7 / 页码印数 `2/39`, `8/39`）**：标称工作温度 `-25°C ~ +85°C`（存储温度 `-40°C ~ +85°C`）。
- <a id="ref-62"></a>**[62]** SeeYA，《SY049WDM01 Specification》，`ingenic/docs/board/SY-SPEC-045P_SEEYA_SY049WDM01_Specification_V1.3_20231128.pdf` — **§2 & §8.1 (p.6, p.15)**：微显示屏标称工作温度 `Topr = -20°C ~ +70°C`，不工作存储温度 `Tstg = -40°C ~ +80°C`。
- <a id="ref-63"></a>**[63]** Ingenic，《X2000 Data Sheet》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_DS_v1.40.pdf` — **§1.2.10 & Table 2-1 (p.10, p.26)**：片上 10-bit SADC 仅含外部 `AUX0~AUX5` 通道，无内置主控温度传感器。
- <a id="ref-64"></a>**[64]** Ingenic，《X2000 Programming Manual》，`ingenic/original_bsp/ingenic/doc/芯片手册/X2000_PM_20230711.pdf` — **Ch. 27《SAR A/D Controller》（正文 p.766–774 / PDF p.800–808）**：SADC 控制器仅支持外部电压采集，无片内结温通道。
- <a id="ref-65"></a>**[65]** 共用 USB/充电子板硬件原理图，`ingenic/docs/board/A-PCB-JK-THG02-USB_VA.pdf`（`VA`）— **p.3–p.4**：`JP1` Type-C 接口与 `U6 BQ25798RQMR` 双 2S 电池升降压充电管理电路。
- <a id="ref-66"></a>**[66]** Texas Instruments，《BQ25798 I2C 1~4 节升降压充电器数据手册》（`ZHCSNY0B`），`ingenic/docs/board/BQ25798RQMR_2227246.pdf` — **§9.5.1.53 `REG41_TDIE_ADC` (p.124)**：内置 16-bit ADC 芯片温度监测寄存器。
- <a id="ref-67"></a>**[67]** InvenSense / TDK，《ICM-20948 Datasheet》，`ingenic/docs/board/ds-000189-icm-20948-v1.5.pdf` — **§8.30–§8.31 `TEMP_OUT_H` / `TEMP_OUT_L` (p.45)**：IMU 片内温度传感器高低字节输出寄存器与换算公式。
- <a id="ref-68"></a>**[68]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **`TODO-11` §11.1–§11.3（L3868–L3947）**：硬件 DDR 带宽监控、多器件板级测温闭环及 72 小时高低温/断电压测验收门禁。
- <a id="ref-69"></a>**[69]** 君正内核 Helix Netlink 用户态码控客户端源码，`ingenic/vendor/kernel/kernel/module_drivers/drivers/media/platform/ingenic-vcodec/helix/h264e_rc_nl.c` — **L155–L158, L249–L285**：默认启用用户态 Netlink 码控，因配套守护进程 `h264e-nl-server` 依赖 `GLIBC_2.29` 在 `glibc 2.28` 根文件系统下直接失效，须切回内核态码控。
- <a id="ref-70"></a>**[70]** 君正内核 V4L2 连续内存缓冲驱动源码，`ingenic/vendor/kernel/kernel/module_drivers/drivers/media/v4l2-core/videobuf2-dma-contig-ingenic.c` — **L656**：`USERPTR` 模式下 `vb2_dc_pfn_to_dma()` 漏加页内偏移 `offset`，导致 `1440×1080` UV 色度平面起始地址错位。
- <a id="ref-71"></a>**[71]** 君正内核 Helix JPEG 硬件编码配置源码，`ingenic/vendor/kernel/kernel/module_drivers/drivers/media/platform/ingenic-vcodec/helix/api/helix_jpeg_enc.c` — **L224–L238**：JPEG 编码按 `mb_height = 68`（`1088` 行）配置硬件 DMA，对未填充对齐的 `1080` 行缓冲产生越界读取。
- <a id="ref-72"></a>**[72]** 君正内核 Helix V4L2 算子源码，`ingenic/vendor/kernel/kernel/module_drivers/drivers/media/platform/ingenic-vcodec/helix/helix_ops.c` — **L266–L273, L851, L1279–L1280**：`vidioc_s_parm` 帧率设置空实现（配合同模块码控静态库 `h264e-rc/librc/librc.a_shipped` 硬编码 `25 fps`）、缓冲区长度校验比较符 `>` 写反及码率上限硬编码锁死 `4 Mbps` 等缺陷。
- <a id="ref-73"></a>**[73]** 君正内核 Helix 内核态码控源码，`ingenic/vendor/kernel/kernel/module_drivers/drivers/media/platform/ingenic-vcodec/helix/h264e_rc.c` — **L397–L402**：`h264e_encode_headers()` 经可缓存 `KSEG0` 地址写入 `SPS/PPS` 未刷回 DDR，引发双核跨核关键帧头部损坏。
- <a id="ref-74"></a>**[74]** 君正内核 X2000 时钟驱动源码，`ingenic/vendor/kernel/kernel/module_drivers/drivers/clk/ingenic-v2/clk-x2000.c` — **L134**：`CLK_DIV_LCD` 像素时钟分频默认向上取整，导致输出行频暴跌 `-12.5%` 触发 OLED 欠压保护关断。
- <a id="ref-75"></a>**[75]** 君正 X2000 MIPI-DSI 硬件抽象层源码，`ingenic/vendor/module_driver/soc/x2000_510/mipi_dsi/jz_mipi_dsi_hal.c` — **L562**：`jz_dsi_video_cfg()` 默认将 `VPG_EN` 置 `0`，导致 `SLPOUT` 唤醒窗口无连续行同步脉冲输出。
- <a id="ref-76"></a>**[76]** 君正 X2000 Framebuffer 显示驱动源码，`ingenic/vendor/module_driver/soc/x2000_510/fb/jzfb.c` — **L404, L421, L911, L1278, L1289**：零帧分配空指针崩溃、默认半透明配置及 `CMD_enable_cfg` 阻断 `yoffset` 双缓冲翻页等缺陷。
- <a id="ref-77"></a>**[77]** 君正 X2000 图层合成器驱动源码，`ingenic/vendor/module_driver/soc/x2000_510/fb_layer_mixer/fb_layer_mixer.c` — **L189, L206**：DMA 描述符缓存同步错误传入指针大小（4 字节）而非结构体长度缺陷。
- <a id="ref-78"></a>**[78]** 君正内核 USB DWC2 Gadget 驱动源码，`ingenic/vendor/kernel/kernel/drivers/usb/dwc2/gadget.c` — **L1014, L1476, L3474**：默认散聚 DDMA 与 UVC 批量传输冲突导致空指针崩溃及同步微帧卡死缺陷。
- <a id="ref-79"></a>**[79]** 君正内核硬件看门狗驱动源码，`ingenic/vendor/kernel/kernel/module_drivers/drivers/watchdog/ingenic_wdt.c` — **L133, L166–L178**：看门狗超时配置条件判断与启停控制缺陷。
- <a id="ref-80"></a>**[80]** 君正内核 DDR 带宽监控驱动源码，`ingenic/vendor/kernel/kernel/arch/mips/xburst2/common/cpu_ddr_test/ddr_bandwidth_monitor.c` — **L444**：旧版定时器接口初始化缺陷及多通道带宽采样实现。
- <a id="ref-81"></a>**[81]** 既有业务仓库 `mini-lite`（`development` 分支 `9b78f6ab0`，独立外部仓）：`thirdparty/rtsp`（ARM 闭源库无法运行于 MIPS32）与 `component/RtspServer/src/src/xop/`（开源 C++11 RTSP 服务模块）。
- <a id="ref-82"></a>**[82]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **`TODO-10` §10.1 & §10.4–§10.7（L3248–L3262, L3461–L3832）**：`mini-lite` 模块复用矩阵、`12 MiB` 零动态分配 5s GOP 环形缓冲、离线分片式 `fMP4` 封装器及 `xop` RTSP 7 项 MIPS32 补丁。
- <a id="ref-83"></a>**[83]** Longsys，《FORESEE eMMC Datasheet — FEMDNN064G-C9A61》，`ingenic/docs/board/eMMC FEMDNN064G-C9A61 Datasheet Version 0.1(2).pdf` — **§3.2.5, §3.3 Table 9 & §4.4 Table 24（PDF p.13, p.24 / 页码印数 `14/39`, `25/39`）**：支持 HS200/HS400 `1.8V` 高速接口，内置 `64 MiB`（`CACHE_SIZE = 10000h`）易失性写缓存。
- <a id="ref-84"></a>**[84]** 君正内核 eMMC 主机控制器驱动源码，`ingenic/vendor/kernel/kernel/module_drivers/drivers/mmc/host/sdhci-ingenic.c`：`MSC0` 8-bit `1.8V` HS200 高速时钟与调谐驱动实现。
- <a id="ref-85"></a>**[85]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **§2.4 & `TODO-02` / `TODO-03`（L344–L391, L712–L1599）**：64GB eMMC `93.75 MHz` HS200 配置、8 MiB 对齐 10 分区只读 GPT 布局及 `2 MiB / 6 MiB` 脏页字节限流加固。
- <a id="ref-86"></a>**[86]** Longsys，《FORESEE eMMC Datasheet — FEMDNN064G-C9A61》，`ingenic/docs/board/eMMC FEMDNN064G-C9A61 Datasheet Version 0.1(2).pdf` — **§1.3 Table 1 & §3.5 Table 11（PDF p.6, p.16 / 页码印数 `7/39`, `17/39`）**：64GB 型号用户区（UDA）可用容量为 `58.28 GB`（`59,680 MiB`）。
- <a id="ref-87"></a>**[87]** 海思量产基线只读分区与系统加固实现，`Hi3559V200_SDK_V2.0.1.0` — **`master_mini` PR#18 / PR#21**：只读 SquashFS/`ext4` 分区挂载、`256 KiB` `ramoops` 崩溃黑匣子归档及电源键 `10s + 3s` 防死机硬件强断保护。
- <a id="ref-88"></a>**[88]** 海思量产基线充电管理驱动，`Hi3559V200_SDK_V2.0.1.0/open_source/linux/linux-4.19.y/drivers/i2c/charger/BQ25798.c` — **`master_mini` PR#13 / PR#16**：`BQ25798` 双 2S 电池充放电管理与 4 层 5% 步进电量算法。
- <a id="ref-89"></a>**[89]** 《Ingenic X2000 Mini-Lite Porting Playbook》，`ingenic/docs/target/TODO_X2000_MINILITE_PORTING.md` — **`TODO-04`（L1600–L1731）**：共用 `JK` 子板外设（`BQ25798` 内外电池 SADC 通道修正、`ICM-20948`、四按键、RTC 与 5 GHz `AIC8800D80` WiFi）移植规范。
- <a id="ref-90"></a>**[90]** 海思量产基线九轴 IMU 驱动，`Hi3559V200_SDK_V2.0.1.0/open_source/linux/linux-4.19.y/drivers/misc/icm20948/icm20948.c` — **`master_mini` PR#15 / PR#21**：`ICM-20948` 内置磁力计轮询与空闲零中断门控实现。
