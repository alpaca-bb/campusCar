# RTK USB / ROS2 Jazzy 接入指南

本文对应 `rtk_gps_driver`，用于把 USB RTK 接收机的 NMEA 数据接入 ROS2，并可选地把 NTRIP RTCM 差分数据写回接收机。

## 1. 先确认 USB 串口

当前这台 NUC 已枚举出四个 AirM2M CDC 串口：`/dev/ttyACM0` 至 `/dev/ttyACM3`，同时存在稳定的 `/dev/serial/by-id/` 链接。不要直接假定四个端口都输出 GNSS，先逐个监视：

```bash
cd /home/qyb413/CyberLuban/campus_road_inspection_ws
python3 -m rtk_gps_driver.serial_monitor --port /dev/ttyACM0 --baudrate 115200
```

如果包还没有安装，直接使用源码路径：

```bash
PYTHONPATH=src/cri_drivers/rtk_gps_driver \
python3 -m rtk_gps_driver.serial_monitor --port /dev/ttyACM0 --baudrate 115200
```

再分别测试 `ttyACM1`、`ttyACM2`、`ttyACM3`。正确的 GNSS 端口会看到类似 `$GNGGA`、`$GPGGA`、`$GNRMC` 的 NMEA 语句；如果收到的是不可打印字节，也会同时显示 `HEX`，这通常是 RTCM 或设备二进制协议。

确认端口后，优先记录 `/dev/serial/by-id/...` 路径，而不是把 `ttyACM0` 写死：

```bash
ls -l /dev/serial/by-id/
```

如果出现 `Permission denied`，把当前用户加入 `dialout` 后重新登录：

```bash
sudo usermod -aG dialout "$USER"
newgrp dialout
```

## 2. 查看收发数据

监视器默认只读串口，不会向 RTK 发送危险配置命令。需要验证发送链路时，显式指定要发送的内容：

```bash
python3 -m rtk_gps_driver.serial_monitor \
  --port /dev/ttyACM0 \
  --baudrate 115200 \
  --send-text $'GPGGA 1\r\n'
```

终端输出含义：

- `RX ...`：USB 串口收到的原始字节，包含 ASCII 显示和 HEX 显示。
- `NMEA GGA`：解析出的经纬度、海拔、定位质量、卫星数和 HDOP。
- `quality=RTK_FIXED`：GGA 质量码 `4`，固定解。
- `quality=RTK_FLOAT`：GGA 质量码 `5`，浮点解。
- 只有看到 `RX`，不代表已经有定位；还要检查 NMEA 中的 GGA 质量码和经纬度。

## 3. 编译 ROS2 Python 包

```bash
source /opt/ros/jazzy/setup.bash
cd /home/qyb413/CyberLuban/campus_road_inspection_ws
colcon build --symlink-install --packages-select rtk_gps_driver
source install/setup.bash
```

若 `python3-serial` 未安装：

```bash
sudo apt install python3-serial
```

## 4. 启动 ROS2 节点

先关闭串口监视器，保证同一个串口没有被两个进程同时打开。然后启动：

```bash
source /opt/ros/jazzy/setup.bash
source /home/qyb413/CyberLuban/campus_road_inspection_ws/install/setup.bash
ros2 launch rtk_gps_driver rtk_gps.launch.py \
  serial_port:=/dev/ttyACM0 \
  baudrate:=115200 \
  ntrip_enabled:=false
```

也可以使用自动枚举：

```bash
ros2 launch rtk_gps_driver rtk_gps.launch.py serial_port:=auto
```

编译完成后，`cri_bringup` 的 `drivers.launch.py` 已经包含 RTK 节点；使用系统总启动时会一起启动：

```bash
ros2 launch cri_bringup drivers.launch.py
```

查看 ROS2 数据：

```bash
ros2 topic echo /gps/nmea
ros2 topic echo /fix
ros2 topic echo /rtk/status
ros2 topic echo /rtk/serial_rx
ros2 topic echo /rtk/serial_tx
ros2 topic hz /fix
```

话题接口如下：

| 话题 | 类型 | 用途 |
| --- | --- | --- |
| `/fix` | `sensor_msgs/msg/NavSatFix` | 标准经纬度、海拔和定位状态 |
| `/gps/nmea` | `std_msgs/msg/String` | 每条原始 NMEA 文本语句 |
| `/rtk/serial_rx` | `std_msgs/msg/UInt8MultiArray` | 串口收到的全部原始字节 |
| `/rtk/serial_tx` | `std_msgs/msg/UInt8MultiArray` | 节点写入串口的全部原始字节，包括 RTCM |
| `/rtk/status` | `std_msgs/msg/String` | JSON 格式的连接、字节计数和定位状态 |

`/fix` 的 `NavSatStatus.status` 映射为：`-1` 无效、`0` 单点、`1` 差分、`2` RTK（固定或浮点）。

## 5. 接入 NTRIP

NTRIP 默认关闭。开启时必须提供服务器、挂载点和账号参数；不要把真实密码提交到 Git：

```bash
ros2 launch rtk_gps_driver rtk_gps.launch.py \
  serial_port:=/dev/ttyACM0 \
  baudrate:=115200 \
  ntrip_enabled:=true \
  ntrip_server:=<服务器地址> \
  ntrip_port:=8002 \
  ntrip_mountpoint:=<挂载点> \
  ntrip_user:=<用户名> \
  ntrip_password:=<密码>
```

工作流是：接收机输出有效 GGA → 节点建立 NTRIP 连接并定期上传 GGA → 接收 RTCM → 通过 USB 写回 RTK 接收机。`/rtk/serial_tx` 可用于确认 RTCM 确实写回硬件。

## 6. 接入 `campusCar` 全栈

`campusCar/scripts/launch_all.sh` 已增加可选启动钩子，默认关闭以保护现有车型流程。确认工作空间编译成功后，在启动前执行：

```bash
cd /home/qyb413/CyberLuban/campusCar
export ROS_SETUP=/opt/ros/jazzy/setup.bash
export RTK_DRIVER_ENABLED=1
export RTK_DRIVER_WS=/home/qyb413/CyberLuban/campus_road_inspection_ws
export RTK_SERIAL_PORT=/dev/ttyACM0
export RTK_BAUD=115200
./scripts/launch_all.sh
```

RTK 驱动日志在 `campusCar/data/logs/nmea_navsat_driver.log`，原有 `u2r_r2u_bridge.py` 会继续订阅 `/fix` 并发布 `/R2UTopic_Pos`。如果只想测试 RTK，不建议先启动全栈，直接使用第 4 节的 `ros2 launch`。

## 7. 常见问题

- **串口被占用**：先退出 `serial_monitor`、旧的 `rtk_gps_node.py` 或其他串口工具，再启动 ROS2 节点。
- **只有 HEX，没有 NMEA**：当前端口可能是 RTCM/配置/蜂窝调制解调器端口，换测其他 `ttyACM*`。
- **有 NMEA 但 `/fix` 没有消息**：检查 GGA 校验和、经纬度字段以及质量码；质量码为 `0` 时默认不发布无效定位。
- **NTRIP 已连接但仍无固定解**：先确认 `/rtk/status` 中 `ntrip_connected=true`，再确认 `/rtk/serial_tx` 的 `data` 持续变化，并检查天线、基站账号和挂载点。
- **找不到 `ros2`**：先执行 `source /opt/ros/jazzy/setup.bash`；若文件不存在，说明 ROS2 Jazzy 没有安装在该路径。
