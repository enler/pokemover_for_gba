Pokémover For GBA
==================

Pokémover For GBA是宝可梦第二世代到第三世代的跨代传宠用GBA自制软件。  
众所周知，宝可梦在第三世代断代了，  
但近几年国外社区已有一些解决方案，可以实现跨代传宠。  
本工具的开发也是受到国外已有成果的启发，  
他们实现了类似第二世代跟第一世代互相交换的时光胶囊的效果，  
能够让第二世代跟第三世代的宝可梦进行双向交换，  
但因为第二世代跟第三世代的数据结构有很大的差异，  
这样的双向交换会使得第三世代的宝可梦的数据大量丢失，  
而且这种方法不能直接传盒子中的宝可梦。  

为了解决这个问题，我们采用了不一样的技术路线进行开发，  
实现了单向传输，并且能够对整个盒子的宝可梦进行操作，使用体验上更接近官方软件。  
另外，因为三代的数据结构与二代有很大的差异，我们在想办法转换这些数据的同时，  
也尽可能确保了传过来的宝可梦在三代游戏中的合法性。  

原理
----

本工具的原理是使用一台gba主机读取gba宝可梦系列的存档，  
然后通过gb(c)连接线连接另外一台作为副机的gba/gbc，  
利用连接线发送传输工具至副机，  
在副机上读取金银水晶的存档，然后将盒子中的宝可梦传到主机中。  

为了顺利发送传输工具，  
副机为gba时，利用了multiboot，以及切换至gb模式的隐藏特性，  
副机为gbc时，利用了速通社区发现的一个水晶版的漏洞，利用这个漏洞实现了远程代码执行  

演示视频
--------

编译
----

先安装devkitpro，然后执行如下命令

    git clone https://github.com/enler/pokemover_for_gba.git
    git submodule update --init

接着使用如下命令编译

    make

开发团队
--------

程序：[enler](https://github.com/enler)  
协力、测试：[卧看微尘](https://github.com/Wokann)  
美术：crossztc  
测试存档提供：海のLUGIA  

以下是开发过程中参考的来自海外社区的成果  

[Goppier](https://github.com/Goppier)：基于自制连接线转接器实现了世界首个跨世代交换方案（硬件解决方案）[见此处](https://www.youtube.com/watch?v=inMbtwmVlKQ)  
[Lorenzooone](https://github.com/Lorenzooone)：基于通信协议模拟实现的纯软件跨世代交换工具[Gen3-to-Gen-X](https://github.com/Lorenzooone/Pokemon-Gen3-to-Gen-X)的开发者（通信协议模拟方案）  
[AntonioND](https://github.com/AntonioND)：gba的gb模式的相关研究，[见此处](https://github.com/AntonioND/gba-switch-to-gbc)  
[pret](https://github.com/pret)：宝可梦游戏反编译工程  
[pkhex](https://github.com/kwsch/PKHeX)：知名的宝可梦存档修改器  
[pokefinder](https://github.com/Admiral-Fish/PokeFinder)：宝可梦乱数工具  
[luckytyphlosion](https://github.com/luckytyphlosion)：宝可梦水晶版任意代码执行漏洞的发现者  
（漏洞也被用于水晶版邪道速通，老实说这个漏洞也是我们在查阅速通的资料中了解到的）  
