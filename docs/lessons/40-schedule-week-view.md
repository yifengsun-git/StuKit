# 第四十课：响应式周课表界面

## 本课结果

课表首页已经从普通课程列表升级为真正的响应式阅读界面：

- 641 像素及以上使用“星期 × 节次”周网格。
- 640 像素及以下切换为按天分组列表。
- 星期标题同时显示实际日期，今天会被轻量强调。
- 课程块按照星期、起始节次和结束节次定位并跨越对应高度。
- 点击课程卡可以查看教师、地点、星期和节次详情。
- 空学期、空教学周、非当前周和查询错误使用不同提示。
- 上一周、下一周、返回当前周和学期切换继续使用同一个 ViewModel。

桌面和未来 Android 界面消费同一份课程模型，没有复制业务查询或单双周规则。

## 1. 为什么日期由 C++ 提供

`ScheduleViewModel::weekDays()` 根据学期开始日期与所选教学周生成七项数据：

```text
weekday, weekdayText, dateText, fullDateText, isToday
```

QML 只负责展示，不自行推算“第 N 周的星期一是哪一天”。这样做有三个好处：

1. 日期计算可以通过固定的 `today` 重复测试。
2. Windows 和 Android 不会分别实现一套日期逻辑。
3. 页面切换周次后，只需重新绑定 `weekDays`。

`WeeklyScheduleListModel` 还增加了 `weekdayCounts`，按星期提供课程数量。窄屏界面据此
显示每日空状态，不需要在 QML 中遍历或复制 C++ 模型。

## 2. QML 组件拆分

本课新增三个组件：

```text
CourseMeetingCard.qml  单个可点击课程卡
WeeklyGrid.qml         桌面星期/节次网格
DayScheduleList.qml    窄屏按天分组列表
```

`ScheduleHomePage.qml` 负责选择使用哪种布局、展示页面级状态并打开课程详情；三个组件
只消费 ViewModel 提供的模型角色。Repository、SQL、单双周判断仍然不进入 QML。

### `CourseMeetingCard.qml`

- 使用课程颜色作为边框和色条，同时保留课程名与节次文字。
- 最小高度遵循 `AppTheme.controlHeight`，可以使用触摸操作。
- 使用一次点击，不依赖悬停、右键或双击。

### `WeeklyGrid.qml`

- 横向七列对应周一至周日。
- 纵向二十行对应第 1～20 节。
- 课程块的 `x` 由星期决定，`y` 和高度由节次决定。
- 网格最小宽度为 900 像素；空间不足时只横向滚动。
- 异常位置数据会先被限制在可显示范围内，避免破坏整个页面。

### `DayScheduleList.qml`

- 七天按日期分组，今天使用边框和文字强调。
- 一天内课程按 C++ 查询结果的稳定顺序排列。
- 无课日期显示简短的“没有课程”。
- 课程卡保持适合触摸的高度。

## 3. 响应式切换

页面通过一个明确断点选择组件：

```qml
readonly property bool compactLayout: width <= 640
readonly property string layoutMode: compactLayout ? "daily" : "grid"
```

`Loader` 同一时间只实例化一种布局。它既减少不必要的对象，也保证数据库恢复并替换
`viewModel` 后，新布局继续绑定新对象。桌面网格由外层页面处理纵向滚动，内部只处理
必要的横向滚动，避免两个方向互相争抢触摸手势。

## 4. 状态区分

页面现在分别处理：

- 没有学期：引导进入“管理学期与课程”。
- 有学期但本周无课：保留日期布局并显示空周提示。
- 查看非当前周：提示用户当前看到的是其他教学周。
- 今天不在所选学期范围：说明无法定位“当前周”的原因。
- 查询失败：使用错误卡片，不把失败误显示成普通空课表。

## 5. 自动测试

新增 `ScheduleQmlIntegrationTest.cpp`，使用内存 Repository 和真实
`ScheduleViewModel` 实例化正式 `ScheduleHomePage.qml`，验证：

- 640 像素使用按天列表。
- 960 和 1280 像素使用周网格。
- 空周可以正常实例化。
- 同一天多门课程和跨多个节次的课程块正确渲染。
- 上下周切换会遵守单双周数据。
- 页面重新绑定另一个 ViewModel 后不会继续显示旧课程。

原有 `SchedulePresentationTest` 同时增加了七天日期和每日课程计数断言。

## 6. 本课验证

- MSVC Debug 完整构建通过。
- `StuKit_qmllint` 无警告通过。
- 新增课表 QML 集成测试通过。
- 完整 CTest 共 46 个测试，全部通过。

## 下一课

[第四十一课：查看模式与学期排课编辑模式](41-schedule-interaction-finish.md)

下一课将当前周查看与整个学期排课分离，并支持直接点击网格新增或修改课程安排。
