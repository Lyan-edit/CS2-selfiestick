# 用户确认的稳定基线：fix5

2026-11-08，用户实测确认：**“抽搐抖动完美解决了，不要有影响并记录当前的状态。”**

基线为 `v0.4.5 prop-origin-fix5`，CS2 build `26445667`，HLAE `2.192.10`。

修改后续机位生命周期之前，已保存完整源码、中英文 DLL、工作区差异及 SHA-256 清单：

`dist/checkpoints/2026-11-08-fix5-user-confirmed-114029/`

- 源码：`fix5-working-source.zip`（68 个文件；逐项校验）。
- 源码包 SHA-256：`A8340A74AB4AE094757D0EAFFB7668ECA14348416080D581F2CEBFD922424241`。
- 中文 DLL：`fix5-zh-CN.dll`，SHA-256 `5ED6B1199C6609A1DDC08FCD3152ADD75CD7BD3C019E74AC7915AC31735B77B8`。
- 英文 DLL：`fix5-en-US.dll`，SHA-256 `34AA660070029889E580DC7938DE4B5E4CD53CF4C365800B0F01ED3C5CE47A30`。
- `manifest.json` 保存所有源码文件及 12 个稳定关键区段的哈希。

后续 fix6 仅更改爆开后的保留时间、锁定/释放策略、界面说明及相应验证。保持引擎绝对坐标重算、坐标读取、刚性机位构图、首次出手方向缓存、实体访问、生命周期出生监听、逐镜头 FOV 和运动诊断实现不变。发布前应逐项对比上述关键区段。

归档不包含个人配置，不覆盖个人 INI，也不修改既有 GitHub v0.4.5 Release。

0.4.6 源码包另外提供 `docs/fix5-stable-sections.json`，仅包含稳定区段哈希及源码包校验值。完整本机归档不随新版重复上传；独立下载源码后，可直接运行 `python tools/check_fix5_preservation.py` 重复核对这 12 个关键区段，无需访问本机归档路径。
