# AXL-V1-A1 物理 TPM Runtime Qualification Kit

本工具包绑定 AxLicense A1 结果 revision：

`a1a37d07d421320d95b7d49ed94ad0ffa7c3cd25`

`axlic.exe` 已由 GitHub 托管 Windows runner 使用冻结 MSVC v143 / CMake 4.4.2 / Windows SDK 10.0.26100.0 预编译。测试机器不负责编译，也不需要安装 Visual Studio、CMake、Git、Windows SDK 或 Python。

## 目标机器要求

- Windows 11 x64 物理机器；
- 以管理员身份运行；
- 已启用且 Ready 的物理 TPM 2.0；
- Microsoft Platform Crypto Provider 可用。

工具包内含官方 Python 3.13.15 x64 embeddable runtime，以及随应用部署的 Microsoft VC143 CRT 运行库。不会安装 Visual Studio Build Tools，也不会自动重启机器。

## 执行步骤

1. 完整解压 ZIP，不要直接在压缩包预览器中运行。
2. 右键 `START-INITIAL.cmd`，选择“以管理员身份运行”。
3. 脚本要求人工确认这是一台物理机器。若检测到 Hyper-V、VMware、VirtualBox、QEMU 等明显虚拟化信号，只会显示 `DRY_RUN_ONLY / NOT_VALID_FOR_T-A1-04`，不能形成阻塞证据。
4. INITIAL 成功后，真实重启 Windows。不要注销、休眠或恢复虚拟机快照。
5. 重启后，以管理员身份运行 `START-POST-REBOOT.cmd`。
6. 成功后只需要把 `output` 目录生成的 `AXL-V1-A1-T-A1-04-Physical-TPM-Evidence-<correlation_id>.zip` 上传回 ChatGPT/Aegis 控制会话。

可随时运行 `VERIFY-KIT.ps1` 检查工具包完整性。工具包和证据不包含私钥、KSP 私密材料、DPAPI 明文或用户身份。

本工具包本身不是 T-A1-04 PASS，不是 P34 PASS，也不表示 READY_FOR_CONTROL_REVIEW。只有真实 Windows 11 物理 TPM 机器完成 initial → 真实重启 → post-reboot，并把证据上传到 reviewer-accessible 路径后，P33 才能重新判断终态。
