# USB_STORAGE

***注意:***

***1.以下配置皆工作在 OTG 模式下，若不使用 OTG，则需要单独配置 host***

***2.硬件上 USB_ID 引脚需拉低***

***3.硬件上 USB_5V 引脚输出电压应大于 4.75V***



## 1.控制器作为 host

## 1.1 menuconfig 配置流程

kernel menuconfig 配置:

```
(1)VFAT (Windows-95) fs support
Symbol: VFAT_FS [=y]
Type  : tristate
Prompt: VFAT (Windows-95) fs support
  Location:
    -> File systems
      -> DOS/FAT/NT Filesystems
  Defined at fs/fat/Kconfig:60
  Depends on: BLOCK [=y]
  Selects: FAT_FS [=y]


(2)Codepage 437 (United States, Canada)
Symbol: NLS_CODEPAGE_437 [=y]
Type  : tristate
Prompt: Codepage 437 (United States, Canada)
  Location:
    -> File systems
      -> Native language support (NLS [=y])
  Defined at fs/nls/Kconfig:39
  Depends on: NLS [=y]


(3)NLS ISO 8859-1  (Latin 1; Western European Languages)
Symbol: NLS_ISO8859_1 [=y]
Type  : tristate
Prompt: NLS ISO 8859-1  (Latin 1; Western European Languages)
  Location:
    -> File systems
      -> Native language support (NLS [=y])
  Defined at fs/nls/Kconfig:318
  Depends on: NLS [=y]

(4)SCSI disk support
Symbol: BLK_DEV_SD [=y]
Type  : tristate
Prompt: SCSI disk support
  Location:
    -> Device Drivers
      -> SCSI device support
  Defined at drivers/scsi/Kconfig:73
  Depends on: SCSI [=y]

(5)
Symbol: USB_STORAGE [=y]
Type  : tristate
Prompt: USB Mass Storage support
  Location:
    -> Device Drivers
      -> USB support (USB_SUPPORT [=y])
        -> Support for Host-side USB (USB [=y])               
  Defined at drivers/usb/storage/Kconfig:8
  Depends on: USB_SUPPORT [=y] && USB [=y] && SCSI [=y]


```

自动挂载U盘到/tmp/mass_storage目录下
<img src="image/1.png" style="zoom:75%;" />


## 1.2 使用方法

1.插入 U 盘弹出打印信息

```
[   46.070034] usb 1-1: new high-speed USB device number 2 using dwc2
[   46.296250] usb 1-1: New USB device found, idVendor=0951, idProduct=1666
[   46.303202] usb 1-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[   46.310592] usb 1-1: Product: DataTraveler 3.0
[   46.315185] usb 1-1: Manufacturer: Kingston
[   46.319508] usb 1-1: SerialNumber: 60A44C413C4EB211A98A00A0
[   46.325821] usb-storage 1-1:1.0: USB Mass Storage device detected
[   46.342363] scsi host0: usb-storage 1-1:1.0
[   47.726378] scsi 0:0:0:0: Direct-Access     Kingston DataTraveler 3.0 PMAP PQ: 0 ANSI: 6
[   47.736215] sd 0:0:0:0: [sda] 30277632 512-byte logical blocks: (15.5 GB/14.4 GiB)
[   47.752538] sd 0:0:0:0: [sda] Write Protect is off
[   47.759990] sd 0:0:0:0: [sda] No Caching mode page found
[   47.765648] sd 0:0:0:0: [sda] Assuming drive cache: write through
[   47.780215]  sda: sda1
[   47.789136] sd 0:0:0:0: [sda] Attached SCSI removable disk
```

2.挂载 U 盘到文件系统

**mount -t vfat /dev/sda1 mnt/**

3.数据交换进入到 /mnt 目录即可操作
