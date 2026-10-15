# 和风天气本机配置

StuKit 使用和风天气的专属 API Host 和 API Key。仓库不保存任何真实密钥，CI 在没有
密钥时仍可完成编译和离线测试；只有实际搜索城市或刷新天气时才需要配置。

## 1. 在和风天气控制台准备凭据

1. 登录[和风天气开发者控制台](https://id.qweather.com/)并创建项目。
2. 在项目设置中复制分配给项目的 **API Host**，只需要类似
   `abc123.qweatherapi.com` 的主机名。
3. 创建 **API Key**。当前客户端按[官方认证说明](https://dev.qweather.com/docs/configuration/authentication/)
   通过 `X-QW-Api-Key` 请求头认证。
4. 不要使用 `api.qweather.com`、`devapi.qweather.com` 或 `geoapi.qweather.com`
   等旧公共 Host。

## 2. Windows 桌面开发配置

Qt Creator 中打开“项目 → 运行 → 运行环境”，增加：

```text
STUKIT_QWEATHER_API_HOST=你的专属 API Host
STUKIT_QWEATHER_API_KEY=你的 API Key
```

重新运行应用即可。也可以只在当前 PowerShell 会话中配置后，从该会话启动程序：

```powershell
$env:STUKIT_QWEATHER_API_HOST = "你的专属 API Host"
$env:STUKIT_QWEATHER_API_KEY = "你的 API Key"
```

环境变量优先级高于 CMake 配置，适合 Windows 调试，也不会进入 Git。

## 3. Android 本机构建配置

Android 应用在手机上不能读取电脑的运行环境变量，因此在 Qt Creator 的 Android Kit
“CMake 配置”中增加：

```text
-DSTUKIT_QWEATHER_API_HOST=你的专属 API Host
-DSTUKIT_QWEATHER_API_KEY=你的 API Key
```

这些值只会进入本机的 `CMakeCache.txt`、构建目录中的生成头文件和最终 APK；不会出现在
编译命令中。`build/` 与 `CMakeCache.txt` 已经被 Git 忽略。修改后应重新运行 CMake，
再重新构建 APK。

注意：这种方式会把 API Key 编译进个人使用的客户端，能够避免误提交 Git，但不能防止
别人从 APK 中提取密钥。若以后公开分发应用，应改成自己的轻量后端签发短期凭据，或按
和风天气[安全指南](https://dev.qweather.com/docs/best-practices/security-guidelines/)使用
服务端认证；不要把长期私钥直接放进 Android 客户端。

## 4. 常见错误

- 提示“请先配置”：Host 或 Key 没有进入当前运行进程，检查 Qt Creator 当前 Kit 的运行
  环境或 CMake 配置。
- 提示“认证失败”：通常是 Host 与 Key 不属于同一个项目、Key 已失效，或复制时包含空格。
- 提示“请求过于频繁”：已经收到 429，等待后重试，不要连续点击刷新。
- 没有网络时：如果该城市有成功缓存，页面继续显示最近缓存及更新时间。

日志不会输出 API Key，也不会把 Key 拼进 URL。
