# Luma3DS 中文版

中文Luma3DS固件（带插件加载器）

### 菜单展示
<div style="float:left;">
  <img style="margin:3px;" src="https://luma3ds-cn.oss-cn-hangzhou.aliyuncs.com/1.jpg" width = "225" height = "300" alt="主菜单一" align=center />
  <img style="margin:3px;" src="https://luma3ds-cn.oss-cn-hangzhou.aliyuncs.com/2.jpg" width = "225" height = "300" alt="主菜单二" align=center />
  <img style="margin:3px;" src="https://luma3ds-cn.oss-cn-hangzhou.aliyuncs.com/3.jpg" width = "225" height = "300" alt="金手指界面" align=center />
  <img style="margin:3px;" src="https://luma3ds-cn.oss-cn-hangzhou.aliyuncs.com/5.jpg" width = "225" height = "300" alt="Action Replay" align=center />
</div>

### 快速安装

- 精简字库版`（推荐）`：
    把精简字库版文件夹下的所有文件及文件夹放入内存卡根目录即可，如果提示覆盖请选是。
    
- ~~完整字库版`（废弃）`：
    把完整字库版文件夹下的所有文件及文件夹放入内存卡根目录即可，如果提示覆盖请选是。~~

### 注意事项

1. boot.firm为luma3DS的主程序文件，请务必放入根目录，luma3DS的原始版本要不低于9.1才可以正常使用，否则请自行搜索如何从低版本升级到9.1版本。（参考链接：https://stray-soul.site/index.php/1-0to1-3 ，感谢火狐大大提供的教程！）

2. unifont_cn.bin或者unifont_full.bin为字库文件，务必放入根目录下的luma文件夹中，不然会无法正常显示中文。

3. cheats文件夹中为进行测试的中文金手指，对应游戏为3d马里奥世界、新马里奥兄弟2、塞尔达三角力量2，把cheat文件夹放在内存卡根目录。（可自行测试，不想测试可以忽略）。

4. 开源软件，禁止任何商业用途！！！

### Q&A：

#### 1. 该中文版Luma3DS改进了什么？
  
> 添加了对中文菜单的支持，优化了菜单的布局顺序及翻页方式，挂载Unicode字库，理论上完整字库版可支持65535个不同字符的显示，金手指也可以根据自己的喜好添加各种不同语言的名称了。
      
#### 2. 两个版本应该如何选择？

> 精简字库版采用国家通用汉字标准（8105个汉字），包含11470个字符，支持英文、简体中文与常用符号显示，满足基本使用需求，且内存占用低，支持老三系列设备，推荐使用该版本！
> 完整字库版为unicode全量字库，包含65535个字符，支持英文、简体中文、繁体中文、韩文、日文、泰文、俄文、等大多数国家的文字与符号显示，但该版本占用内存较多。
> V12.0版本由于增加了新功能，引入了一些其他标准库，导致编译文件体积增大，因内存有限，完整版字库暂时不能支持了，目前仅支持精简字库版本。

#### 3. 为何选用带插件加载器的版本进行汉化？

> 因为该版本基于最新的版本进行优化，功能和稳定性都有提升，且BootNTR自制固件、3gx插件、GW金手指及最新版本操作系统都得到了很好的兼容，所以选用该版本进行汉化，在这里感谢Nanqiutas大神。

#### 4. 安装后发现BootNTR自制固件不能加载了怎么办？

> 出现该问题可能是由于版本不匹配导致的，建议使用最新的BootNTRSelector。
> 下载地址：https://github.com/Nanquitas/BootNTR/releases
> 注：由于BootNTR固件占用内存较高，在老三设备上，经过一番又一番优化后也没能解决内存占用的问题，因此，老三设备的BootNTR固件目前暂时不能与中文版Luma3DS同时使用，在这里说声抱歉！如果有大佬能解决内存占用高的问题，希望可以在百忙中伸出援手帮我解决这个问题，不胜感激，谢谢！

#### 5.菜单中的插件加载选项是干什么用的？

> 插件加载功能是由Nanquitas大神基于Luma3DS的基础上进行开发的，可以加载3gx格式的插件以实现更多具有特色的功能，比较出名的插件有ActionReplay，它具有搜索内存地址，添加金手指、更改金手指热键等功能，不过该插件目前是英文的，大家可根据需求选择使用。

#### 6.如何使用插件加载功能？

> 以[ActionReplay v0.6.0](https://github.com/CynricXu/CTRPluginFramework-BlankTemplate/releases/tag/v0.6.0) 为例(该版本为mariohackandglitch最终提交修改)，将插件ActionReplay.3gx更名为default.3gx放入/luma/plugins/下，确保Luma3DS菜单中的插件加载选项为开启状态，进入游戏即可使用select按键呼出（可更改热键）。

### 关于Luma3DS

&emsp;**Luma3DS**是一个程序，用于“动态”修补（新）任天堂（2）3DS掌上游戏机的系统软件，添加诸如按游戏语言设置、为开发者调试功能等功能，并取消任天堂强制实施的限制，如区域锁。

&emsp;它还允许您通过删除签名检查运行未经授权的“自制”内容，要使用它，您需要一个能够在Arm9处理器上运行自制软件的控制台。

&emsp;从v8.0开始，Luma3DS有了自己的游戏内菜单，可以通过<kbd>L+Down+Select</kbd>触发（参见[发行说明](https://github.com/LumaTeam/luma3s/releases/tag/v8.0)）。

### 编译
* Prerequisites
    1. git
    2. [makerom](https://github.com/jakcron/Project_CTR) in PATH
    3. [firmtool](https://github.com/TuxSH/firmtool)
    4. Up-to-date devkitARM+libctru
1. Clone the repository with `git clone https://github.com/LumaTeam/Luma3DS.git`
2. Run `make`.

    The produced `boot.firm` is meant to be copied to the root of your SD card for usage with Boot9Strap.

### 致谢
查看 https://github.com/LumaTeam/Luma3DS/wiki/Credits

### Licensing
This software is licensed under the terms of the GPLv3. You can find a copy of the license in the LICENSE.txt file.

Files in the GDB stub are instead triple-licensed as MIT or "GPLv2 or any later version", in which case it's specified in the file header.
