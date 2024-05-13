// port from https://github.com/kwsch/PKHeX/blob/master/PKHeX.Core/PKM/Strings/StringConverter12Transporter.cs
#include "characters.h"
#include "define_gsc.h"
#include "global.h"
#include "math_fast.h"

#define CRYSTAL_CHS_KANJI_BEGIN 0x04C3

struct CrystalCHSSymbol {
    u16 origCharCode;
    u8 charCode;
    bool8 isReplaced;
};

static const u8 CharTableIntl[] = __("                "
                                     "                "
                                     "                "
                                     "                "
                                     "                "
                                     "$               "
                                     "                "
                                     "                "
                                     "ABCDEFGHIJKLMNOP"
                                     "QRSTUVWXYZ():;()"
                                     "abcdefghijklmnop"
                                     "qrstuvwxyz     Á"
                                     "ÄÖÜäöü   Í      "
                                     "                "
                                     "'PM-  ?!.      ♂"
                                     " x./,♀0123456789");

static const u8 CharTableJPN[] = __("　　　　　ガギグゲゴザジズゼゾダ"
                                    "ヂヅデド　　　　　バビブボ　　　"
                                    "　　　　　　がぎぐげござじずぜぞ"
                                    "だぢづでど　　　　　ばびぶベぼ　"
                                    "パピプポぱぴぷペぽ　　　　　　　"
                                    "$　　　　　　　　　　　　　　　"
                                    "　　　　　　　　　　　　　　　　"
                                    "　　　　　　　　　　　　　　　　"
                                    "アイウエオカキクケコサシスセソタ"
                                    "チツテトナニヌネノハヒフホマミム"
                                    "メモヤユヨラルレロワヲンッャュョ"
                                    "ィあいうえおかきくけこさしすせそ"
                                    "たちつてとなにぬねのはひふヘほま"
                                    "みむめもやゆよらリるれろわをんっ"
                                    "ゃゅょー　　?!　ァゥェ　　　♂"
                                    "　　　　ォ♀0123456789");

static const u8 CharTableGSCHS[] = __("妙蛙种子草花小火龙恐喷杰尼龟卡咪"
                                      "水箭绿毛铁甲蛹巴大蝶独角壳针蜂波"
                                      "比鸟拉达烈雀嘴阿柏蛇怪皮丘雷穿山"
                                      "鼠王多兰娜后朗力诺可西访尾九胖丁"
                                      "超音蝠走路臭霸派斯特球摩鲁蛾地层"
                                      "喵猫老鸭哥猴暴蒂狗风速蚊香蝌蚪君"
                                      "泳士凯勇基胡腕豪喇叭芽口呆食玛瑙"
                                      "母毒刺拳石隆岩马焰兽磁合怜葱嘟利"
                                      "海狮白泥舌贝鬼通耿催眠貘引梦人钳"
                                      "蟹巨霹雳电顽弹维椰五嘎啦飞腿郎快"
                                      "头瓦双犀牛钻吉蔓藤袋墨金鱼星宝魔"
                                      "墙偶天螳螂迷唇姐击罗肯泰鲤普百变"
                                      "伊布边菊化盔镰刀翼急冻闪你哈克幻"
                                      "叶月桂竺葵锯鳄蓝立咕夜鹰芭瓢安圆"
                                      "丝蛛叉字灯笼古然羊"
                                      "茸才露丽皇毽棉长手向祈蜻蜓乌沼太"
                                      "阳亮黑暗鸦未知图腾果翁麒麟奇榛佛"
                                      "托土弟蝎钢千壶赫狃熊圈熔蜗猪关瑚"
                                      "炮章桶信使翅戴加象顿型惊鹿犬无畏"
                                      "战舞娃奶罐幸福公炎帝幼沙班洛亚凤"
                                      "时拍空劈连环掌续万吨重聚功冰抓夹"
                                      "住断旋剑居斩起膀攻吹翔绑紧摔打鞭"
                                      "踩踏趟踢回泼锤撞乱压顶束猛闹番舍"
                                      "身冲摇瞪眼咬叫声吼唱歌爆定法溶解"
                                      "液射雾枪浪光雪线泡沫极破坏啄狱翻"
                                      "滚倒倍奉还上投吸取级寄生粉麻痹瓣"
                                      "吐之怒涡十伏落震裂挖洞剧念精神强"
                                      "术瑜伽姿势高移动愤瞬间影模仿耳别"
                                      "自我再硬烟幕异缩入中屏障反壁气忍"
                                      "耐挥指鹦鹉学炸舔浊"
                                      "骨棒攀瀑尖农缠绕失忆折弯汤匙膝血"
                                      "恶吻昏蘑孢跃镖睡觉崩必杀门牙棱纹"
                                      "理开替挣扎写偷网心轮鼾诅咒狂死怨"
                                      "恨细守面出腹鼓掷撒菱识同命灭亡看"
                                      "锁逆鳞终挺娇点到为止虚张喝色目话"
                                      "治愈铃报恩礼物迁做护担痛楚圣息接"
                                      "来次追转甜属爪借晨作用醒量卷求雨"
                                      "晴碎镜示原始预潮围鉴背包记录设置"
                                      "珊闭齿师灵行车药灼伤全复满厉害好"
                                      "离绳除项补充剂增防御度糖珠玩活片"
                                      "块能银黄要美味劲爽汽汁代币盒探器"
                                      "习装旧钓竿提升单红传船票透明铛羽"
                                      "哞鲜先制的实锐柔软消烧烤者证苦涩"
                                      "薄荷符洁净滴曲带不融迹珍馒粗元根"
                                      "运钥机械零件遗下木"
                                      "炭焦膜吃剩东因灰沉案邮等诱饵友蜜"
                                      "箱桐玉数据虹砖肖像爱继冒险新更改"
                                      "钟已获得徽个当前只主对斗画规则菜"
                                      "说窗慢交换道体显保存吗正在请切源"
                                      "完了将覆盖问以另份并内容发损若需"
                                      "本每候就会载详查状态放各类具训练"
                                      "家导航含有工游戏位持返招式列表般"
                                      "格幽亲性经验值病品选择哪教里四雄"
                                      "任何名钱镇市桔梗桧缘朱浅湛湖塔穴"
                                      "井台厂辉漩岛擂钵真常青深华紫苑枯"
                                      "莲英冠军见隧都尔司狩猎屋窟号栎林"
                                      "流森城雌树按意键想给谁呢删弃搜索"
                                      "韩文顺序进标准编捕版现威逃跑丢登"
                                      "否是捉最较概率该降低方够两叠让疗"
                                      "恢部所步遇弱础易和"
                                      "纯造商店卖价脱与野濒半从清很隐藏"
                                      "应其他共垂于稍微而闻技健康搭乘券"
                                      "爷那什么芒富营养陷怕混贵售难分产"
                                      "某几额外启广播帘少女幅但印卉纸越"
                                      "密植浇彩样脑殿堂整呼系统评受成秘"
                                      "没封即这被塞己第收丛寻找吧嗯去如"
                                      "擅远努善靠响喜欢集些逐坚哇趣它们"
                                      "帮助仔研究朋过啊错待非博专祝贺阅"
                                      "读送足便调确题隔久校跳或殴拿寒锋"
                                      "侧剪刃刮袭展结脚予穷败脸伸扑喙也"
                                      "许拼晃听殊阻酸掀巧甩相浑末散困松"
                                      "轻盈怖讨厌避蜷扰固减激遍肮形尽爬"
                                      "把触忘事注掉胞父孩耀蹦敲峰段黏感"
                                      "噜效抵抗贴怀迅着疏备近周冷瞄凝视"
                                      "留伴舒适优秀泄平敌"
                                      "虽兴附团参挑尘计纱仍停奖励胜试妈"
                                      "输途捆处懒令拔群伙配吞境牢承抢夺"
                                      "噩削照昵称三楼买床毯盆饰壮观南国"
                                      "杯右左惜绩北局早晚讲座乐频往语恭"
                                      "此颁资距底馆员短裤裙队绅滑年科男"
                                      "族趁劫杂耍艺餐尚婆俱服胎警察谜睁"
                                      "唔抱歉哎呀今搞余码决嘛喂昨购房告"
                                      "叮咚稀又缆望谢考虑哦拥算情旅管怎"
                                      "迎油勉培育故吓暂领费儿摄姓思似干"
                                      "授误砍泻由推堵弄骑棵漆孵刚乎世界"
                                      "尊敬互直艰随范协临廉呵您宜办忙凑"
                                      "齐认洗牌差添蛋笛节辛休书摆志笔条"
                                      "赚坛玻璃页联诉憾付桃栖副总颜艳丑"
                                      "河义胆魄燃伟欲篮杨疲劳诗振奋吵剔"
                                      "议敢啼建组织宣坂扩"
                                      "赶务渐烦仅谈际旺盛扮漂聊举占亏修"
                                      "米柱雕塌雅施材约严哼毕竟毫检牵绊"
                                      "哉须愿诸赖熟碍废惨魂悟炽热温敏捷"
                                      "刻糟糕惯呜坪例赢永疼盯妨彰宏谓贞"
                                      "夫嘘闲智兑核允答插料档希她划业室"
                                      "噗禁迫党陪闷谅一愉救站扣突陌摸嗷"
                                      "堡垒句忠及撼演假货仓库欣赏昔荣拯"
                                      "曾跟板绘睛静筑陵首测骗肚饿默辞喊"
                                      "腰割饶爸悠至纪逛既锻炼日祷睐绝佳"
                                      "洋岸梯嘿渡侵监控碰埋阱轰旁兵汹涌"
                                      "执凶坦篷披言论哀叹岁磨傅阴篇枝繁"
                                      "祠供弊骚勤兄帅历史铺区域烂捡酷厚"
                                      "搬哟蒙操甘椿盟朝朵捍卫荡危莫挡匠"
                                      "谐欠旱灾丈叔批卜瞎茜灿致笨汪遥港"
                                      "蹈客毁勿尝拾乡柳傲"
                                      "琪初端径凡坐宁携躲怡妹漫却汇抖躁"
                                      "沾排嗡握巡疑措仇滩彻烫鼻踪猜七茉"
                                      "奈惧悲顾窍呗牧哐炫潜晕浮暖累肤晒"
                                      "霜脏涂诡胸宽阔渺捣荐赔貌患悦眉愧"
                                      "偏质免归介绍惩罚诞茫蔬挤李笑冬春"
                                      "溜诀肉衣宫胶囊抬奴奏简饱遵矩季辅"
                                      "魅羡慕夏秋暑懂疾嗨驱兆贸佩况吊贩"
                                      "孤昆奥莓创邪聪瞌扭限赛仪哭辆仗社"
                                      "策略饮缓积虎款判钩狠纽汉惑屿程赠"
                                      "扔询户邻槽映坑耗妇负责缺乏迈鸣哨"
                                      "澡貘赤陆素斑江泊汗杏垃圾嘲坡池厦"
                                      "胃销莉民践课衡啪嗒跪嘻栅栏寂桥悔"
                                      "扫墓唤尤征橙尴尬夸羞淑沿霞俏盗窃"
                                      "词忽括云旦盘撕构职冥闯犯孙诚聆茶"
                                      "梳苟延残喘梨恼采毅"
                                      "掩泣恋旷伪抛律抽填拖斜呃碗忧景痕"
                                      "竞蜥蜴蝴狐狸蝙鼹暹蝇憨锹拟芳颚躯"
                                      "琵琶绵兔颈蓑酵筝铠沐浴苞惹寿苔藓"
                                      "释脆妄巢肌扇殖啃筋促咧慧储著奔扬"
                                      "犄崽嚼遭竖硕辨贮胀依偎稠湿煽掘溢"
                                      "蕴歪脖蹼肢糊鬃凛横苗屈臂歇瘦泌浓"
                                      "退争茎凉呈堆悄逼裹凹凸漏架寞络卓"
                                      "甚泪咳嗽涕膨浆迟钝鳍卵染央哑娴绪"
                                      "尺骤颤蜕溺浩瀚淡抚怯懦颗戒烁翩臀"
                                      "凭蓄讶廓丰脂川倾湍拂纵悬支撑伫觅"
                                      "六孔忌讳慌唬良粘乳渗擦泉畅觊觎渣"
                                      "翱沛陶醉抡抑涨茂蕾宛贯巩磷洒砾碌"
                                      "颊燥勒脊椎庞$?壤耕田吠摁剥蜇峻踹"
                                      "蔽蒸厘纠纷菌卧攒粒坟唾痒澈蓬吮亢"
                                      "袤晶焕萎苏匍匐屁唯"
                                      "眩橡稳恫扯峭扇循嗅礁雏涛巅荆棘哺"
                                      "弘樱兼晋昊鸿德捷祺逸淳彦宪宇$?咏"
                                      "馨妍芷$?蕙贤拓绯惠堇俊嗣臣乔琴卿"
                                      "武仁哲漳庄轩沧泓洪钧润宗灏渊庆晓"
                                      "渚帆湄郁汐雁政滋恒骏吾铅昭峥崇嵘"
                                      "犹藏昌勋$?裕肇舜蕊蓁铭嫣霓裳典晖"
                                      "梅痊矢八匕哆佐寸巾乞亿夕勺丸咩污"
                                      "期场且拜伯岭股戈曰冈壬夭仆斤虫妖"
                                      "菇园享午飘逻锥昶刊扒艾丙夯戊轧卢"
                                      "申叽叼叩叨冉皿囚二乍禾仙斥瓜匆册"
                                      "卯饥冯玄讯弗辽召孕矛邦迂刑戎扛寺"
                                      "芋芝朽朴权吏戌夷轨尧劣吁吕吆屹岂"
                                      "廷竹迄乒乓伍臼伐仲伦仰瑕绚栋枚煌"
                                      "署肥旭玲圭麦瑞阶闸漾肆虐阵缚均彼"
                                      "套财罢匹铜柜磐亘翠");

static const u8 CharTableGSCHS_ABC[] = __("ABCDEFGHJKLMNOPQ"
                                          "RST");

static const u8 CharTableGSCHS_UVW[] = __("UVWXYZabcdefghij"
                                          "kmnrt");

static const struct CrystalCHSSymbol CharTableCrystalCHS_Ext[] = {
    {0x0101, CHAR_SPACE, FALSE},           // '　'
    {0x0102, CHAR_COMMA, TRUE},            // '、'
    {0x0103, CHAR_PERIOD, TRUE},           // '。'
    {0x0104, CHAR_BULLET, FALSE},          // '·'
    {0x010A, CHAR_HYPHEN, FALSE},          // '-'
    {0x010B, CHAR_HYPHEN, TRUE},           // '~'
    {0x010D, CHAR_ELLIPSIS, FALSE},        // '…'
    {0x010E, CHAR_SGL_QUOTE_LEFT, FALSE},  // '‘'
    {0x010F, CHAR_SGL_QUOTE_RIGHT, FALSE}, // '’'
    {0x0110, CHAR_DBL_QUOTE_LEFT, FALSE},  // '“'
    {0x0111, CHAR_DBL_QUOTE_RIGHT, FALSE}, // '”'
    {0x0112, CHAR_SGL_QUOTE_LEFT, TRUE},   // '〔'
    {0x0113, CHAR_SGL_QUOTE_RIGHT, TRUE},  // '〕'
    {0x0117, CHAR_SGL_QUOTE_LEFT, TRUE},   // '<'
    {0x0118, CHAR_SGL_QUOTE_RIGHT, TRUE},  // '>'
    {0x0119, CHAR_DBL_QUOTE_LEFT, TRUE},   // '《'
    {0x011A, CHAR_DBL_QUOTE_RIGHT, TRUE},  // '》'
    {0x011B, CHAR_SGL_QUOTE_LEFT, TRUE},   // '「'
    {0x011C, CHAR_SGL_QUOTE_RIGHT, TRUE},  // '」'
    {0x011D, CHAR_DBL_QUOTE_LEFT, TRUE},   // '『'
    {0x011E, CHAR_DBL_QUOTE_RIGHT, TRUE},  // '』'
    {0x011F, CHAR_DBL_QUOTE_LEFT, TRUE},   // '〖'
    {0x0120, CHAR_DBL_QUOTE_RIGHT, TRUE},  // '〗'
    {0x0121, CHAR_DBL_QUOTE_LEFT, TRUE},   // '【'
    {0x0123, CHAR_DBL_QUOTE_RIGHT, TRUE},  // '】'
    {0x0125, CHAR_x, FALSE},               // 'x'
    {0x01D7, CHAR_EXCL_MARK, FALSE},       // '！'
    {0x01DE, CHAR_SGL_QUOTE_LEFT, TRUE},   // '('
    {0x01DF, CHAR_SGL_QUOTE_RIGHT, TRUE},  // ')'
    {0x01E2, CHAR_COMMA, FALSE},           // '，'
    {0x01E4, CHAR_PERIOD, FALSE},          // '．'
    {0x01E5, CHAR_SLASH, FALSE},           // '/'
    {0x01E6, CHAR_0, FALSE},               // '０'
    {0x01E7, CHAR_1, FALSE},               // '１'
    {0x01E8, CHAR_2, FALSE},               // '２'
    {0x01E9, CHAR_3, FALSE},               // '３'
    {0x01EA, CHAR_4, FALSE},               // '４'
    {0x01EB, CHAR_5, FALSE},               // '５'
    {0x01EC, CHAR_6, FALSE},               // '６'
    {0x01ED, CHAR_7, FALSE},               // '７'
    {0x01EE, CHAR_8, FALSE},               // '８'
    {0x01EF, CHAR_9, FALSE},               // '９'
    {0x01F0, CHAR_COLON, FALSE},           // '：'
    {0x01F1, CHAR_COLON, TRUE},            // '；'
    {0x01F5, CHAR_QUESTION_MARK, FALSE},   // '？'
    {0x01F7, CHAR_A, FALSE},               // 'Ａ'
    {0x01F8, CHAR_B, FALSE},               // 'Ｂ'
    {0x01F9, CHAR_C, FALSE},               // 'Ｃ'
    {0x01FA, CHAR_D, FALSE},               // 'Ｄ'
    {0x01FB, CHAR_E, FALSE},               // 'Ｅ'
    {0x01FC, CHAR_F, FALSE},               // 'Ｆ'
    {0x0201, CHAR_G, FALSE},               // 'Ｇ'
    {0x0202, CHAR_H, FALSE},               // 'Ｈ'
    {0x0203, CHAR_I, FALSE},               // 'Ｉ'
    {0x0204, CHAR_J, FALSE},               // 'Ｊ'
    {0x0205, CHAR_K, FALSE},               // 'Ｋ'
    {0x0206, CHAR_L, FALSE},               // 'Ｌ'
    {0x0207, CHAR_M, FALSE},               // 'Ｍ'
    {0x0208, CHAR_N, FALSE},               // 'Ｎ'
    {0x0209, CHAR_O, FALSE},               // 'Ｏ'
    {0x020A, CHAR_P, FALSE},               // 'Ｐ'
    {0x020B, CHAR_Q, FALSE},               // 'Ｑ'
    {0x020C, CHAR_R, FALSE},               // 'Ｒ'
    {0x020D, CHAR_S, FALSE},               // 'Ｓ'
    {0x020E, CHAR_T, FALSE},               // 'Ｔ'
    {0x020F, CHAR_U, FALSE},               // 'Ｕ'
    {0x0210, CHAR_V, FALSE},               // 'Ｖ'
    {0x0211, CHAR_W, FALSE},               // 'Ｗ'
    {0x0212, CHAR_X, FALSE},               // 'Ｘ'
    {0x0213, CHAR_Y, FALSE},               // 'Ｙ'
    {0x0217, CHAR_Z, FALSE},               // 'Ｚ'
    {0x0218, CHAR_SGL_QUOTE_LEFT, TRUE},   // '［'
    {0x0219, CHAR_SLASH, TRUE},            // '\'
    {0x021A, CHAR_SGL_QUOTE_RIGHT, TRUE},  // '］'
    {0x021E, CHAR_a, FALSE},               // 'ａ'
    {0x021F, CHAR_b, FALSE},               // 'ｂ'
    {0x0220, CHAR_c, FALSE},               // 'ｃ'
    {0x0221, CHAR_d, FALSE},               // 'ｄ'
    {0x0223, CHAR_e, FALSE},               // 'ｅ'
    {0x0224, CHAR_f, FALSE},               // 'ｆ'
    {0x0225, CHAR_g, FALSE},               // 'ｇ'
    {0x0226, CHAR_h, FALSE},               // 'ｈ'
    {0x0227, CHAR_i, FALSE},               // 'ｉ'
    {0x0228, CHAR_j, FALSE},               // 'ｊ'
    {0x0229, CHAR_k, FALSE},               // 'ｋ'
    {0x022A, CHAR_l, FALSE},               // 'ｌ'
    {0x022B, CHAR_m, FALSE},               // 'ｍ'
    {0x022C, CHAR_n, FALSE},               // 'ｎ'
    {0x022D, CHAR_o, FALSE},               // 'ｏ'
    {0x022E, CHAR_p, FALSE},               // 'ｐ'
    {0x022F, CHAR_q, FALSE},               // 'ｑ'
    {0x0230, CHAR_r, FALSE},               // 'ｒ'
    {0x0231, CHAR_s, FALSE},               // 'ｓ'
    {0x0232, CHAR_t, FALSE},               // 'ｔ'
    {0x0233, CHAR_u, FALSE},               // 'ｕ'
    {0x0234, CHAR_v, FALSE},               // 'ｖ'
    {0x0235, CHAR_w, FALSE},               // 'ｗ'
    {0x0236, CHAR_x, FALSE},               // 'ｘ'
    {0x0237, CHAR_y, FALSE},               // 'ｙ'
    {0x0238, CHAR_z, FALSE},               // 'ｚ'
    {0x0239, CHAR_SGL_QUOTE_LEFT, TRUE},   // '｛'
    {0x023B, CHAR_SGL_QUOTE_RIGHT, TRUE}   // '｝'
};

static const u8 InvalidNameCharsIntl[][2] = {
    {CHAR_LEFT_PAREN, CHAR_SGL_QUOTE_LEFT},
    {CHAR_RIGHT_PAREN, CHAR_SGL_QUOTE_RIGHT},
    {CHAR_SEMICOLON, CHAR_QUESTION_MARK},
    {CHAR_COLON, CHAR_QUESTION_MARK},
    {CHAR_A_ACUTE, CHAR_A},
    {CHAR_I_ACUTE, CHAR_I},
};

static const u8 InvalidNameCharsCHS[][2] = {
    {CHAR_LEFT_PAREN, CHAR_SGL_QUOTE_LEFT},
    {CHAR_RIGHT_PAREN, CHAR_SGL_QUOTE_RIGHT},
    {CHAR_SEMICOLON, CHAR_COLON},
    {CHAR_A_ACUTE, CHAR_A},
    {CHAR_I_ACUTE, CHAR_I},
};

static const u8 ValidNameCharsGerman[][2] = {
    {CHAR_A_DIAERESIS, CHAR_A},
    {CHAR_O_DIAERESIS, CHAR_O},
    {CHAR_U_DIAERESIS, CHAR_U},
    {CHAR_a_DIAERESIS, CHAR_a},
    {CHAR_o_DIAERESIS, CHAR_o},
    {CHAR_u_DIAERESIS, CHAR_u},
};

static void ConvertSpecificKana(u8 *dest, size_t destLen) {
    bool8 containsSpecificKata = FALSE;
    int i;
    for (i = 0; i < destLen && dest[i] != EOS; i++) {
        if (dest[i] == 0x6D || // 'ヘ'
            dest[i] == 0x78 || // 'リ'
            dest[i] == 0x99 || // 'ベ'
            dest[i] == 0x9E)   // 'ペ'
        {
            containsSpecificKata = TRUE;
            break;
        }
    }
    if (containsSpecificKata) {
        for (i = 0; i < destLen && dest[i] != EOS; i++) {
            if (dest[i] >= 0x01 && dest[i] <= 0x50)
                break;
            else if (dest[i] >= 0x51 && dest[i] <= 0xA0)
                return;
        }

        for (i = 0; i < destLen && dest[i] != EOS; i++) {
            if (dest[i] == 0x6D || // 'ヘ'
                dest[i] == 0x78 || // 'リ'
                dest[i] == 0x99 || // 'ベ'
                dest[i] == 0x9E)   // 'ペ'
                dest[i] -= 0x50;
        }
    }
}

static u16 DecodeCrystalCHSChar(u16 charCode) {
    u8 hi, lo;
    s32 i, left, right;
    static const u8 loSkippedTbl[][2] = {
        {0x0560 - CRYSTAL_CHS_KANJI_BEGIN, 30},
        {0x0540 - CRYSTAL_CHS_KANJI_BEGIN, 9},
        {0x0523 - CRYSTAL_CHS_KANJI_BEGIN, 8},
        {0x0517 - CRYSTAL_CHS_KANJI_BEGIN, 7},
        {0x0501 - CRYSTAL_CHS_KANJI_BEGIN, 4}
    };
    static const u8 hiSkippedTbl[][2] = {
        {(0x2801 - CRYSTAL_CHS_KANJI_BEGIN) >> 8, 12},
        {(0x1801 - CRYSTAL_CHS_KANJI_BEGIN) >> 8, 4}
    };
    hi = charCode >> 8;
    lo = charCode & 0xFF;
    if (charCode >= CRYSTAL_CHS_KANJI_BEGIN &&
        ((hi >= 0x4 && hi <= 0x13) ||
         (hi >= 0x18 && hi <= 0x1F) ||
         (hi >= 0x28 && hi <= 0x2E))) {
        charCode -= CRYSTAL_CHS_KANJI_BEGIN;
        if (lo == 0x0 ||
            (lo >= 0x14 && lo <= 0x16) ||
            lo == 0x22 || lo == 0x3F ||
            (lo >= 0x4B && lo <= 0x5F) ||
            (lo >= 0xFD && lo <= 0xFF) ||
            (hi == 0x19 && (lo >= 0x6C && lo <= 0x70)))
        {
            return CHAR_QUESTION_MARK;
        }

        hi = charCode >> 8;
        lo = charCode & 0xFF;

        for (i = 0; i < ARRAY_COUNT(loSkippedTbl); i++) {
            if (lo >= loSkippedTbl[i][0]) {
                lo -= loSkippedTbl[i][1];
                break;
            }
        }

        for (i = 0; i < ARRAY_COUNT(hiSkippedTbl); i++) {
            if (hi >= hiSkippedTbl[i][0]) {
                hi -= hiSkippedTbl[i][1];
                break;
            }
        }
        
        ConvertFromBase226ToBase247Customized(&hi, &lo);
        hi++;
        if (hi >= 0x6)
            hi++;
        if (hi >= 0x1B)
            hi++;
        charCode = (hi << 8) | lo;
    }
    else {
        left = 0;
        right = ARRAY_COUNT(CharTableCrystalCHS_Ext) - 1;
        while (left <= right) {
            i = left + (right - left) / 2;
            if (CharTableCrystalCHS_Ext[i].origCharCode == charCode) {
                return CharTableCrystalCHS_Ext[i].charCode;
            }
            if (CharTableCrystalCHS_Ext[i].origCharCode > charCode) {
                right = i - 1;
            }
            else {
                left = i + 1;
            }
        }
        charCode = CHAR_QUESTION_MARK;
    }
    return charCode;
}

static u16 DecodeGSCHSChar(u16 charCode) {
    static const u8 loSkippedTbl0[][2] = {
        {0xA1, 21},
        {0x60, 19},
        {0x31, 3},
        {0x16, 1}
    };
    static const u8 loSkippedTbl1[][2] = {
        {0xFF, 23},
        {0xD1, 22},
        {0x71, 20},
        {0x60, 18},
        {0x16, 2},
        {0x1, 1}
    };
    const u8 (*loSkippedTbl)[2];
    s32 i, tableSize, seq;
    u16 lo;
    u8 hi = charCode >> 8;
    if (hi >= 0x1 && hi < 0xB) {
        loSkippedTbl = charCode & 0x100 ? loSkippedTbl1 : loSkippedTbl0;
        tableSize = charCode & 0x100 ? ARRAY_COUNT(loSkippedTbl1) : ARRAY_COUNT(loSkippedTbl0);
        lo = charCode & 0x1FF;
        if ((lo & 0xFF) == 0x15 || 
            lo == 0x2F || lo == 0x30 ||
            ((lo & 0xFF) >= 0x50 && (lo & 0xFF) <= 0x5F) ||
            lo == 0x9F || lo == 0xA0 ||
            (lo >= 0xFE && lo <= 0x100) ||
            lo == 0x16F || lo == 0x170 ||
            lo == 0x1CF || lo == 0x1D0 || lo == 0x1FE)
                return CHAR_QUESTION_MARK;
        lo = lo & 0xFF;
        for (i = 0; i < tableSize; i++)
        {
            if (lo >= loSkippedTbl[i][0]) {
                lo -= loSkippedTbl[i][1];
                break;
            }
        }

        seq = (hi - 1) * 233 + lo;
        hi = CharTableGSCHS[seq * 2];
        lo = CharTableGSCHS[seq * 2 + 1];
        if (hi == EOS)
            charCode = lo;
        else
            charCode = (hi << 8) | lo;
    }
    else if (hi == 0xB) {
        lo = charCode & 0xFF;
        if (lo < 0x13)
            charCode = CharTableGSCHS_ABC[lo];
        else if (lo >= 0x20 && lo < 0x35) 
            charCode = CharTableGSCHS_UVW[lo - 0x20];
        else if (lo == 0xFF)
            charCode = CHAR_SPACE;
        else
            charCode = CHAR_QUESTION_MARK;
    }
    else {
        charCode = CHAR_QUESTION_MARK;
    }

    return charCode;
}

bool8 ContainsCrystalCHSChars(const u8 *str) {
    u8 hi, lo;
    u16 charCode, charCodeAlt;
    bool8 result = TRUE;
    bool8 matched = FALSE;
    while (*str != 0x50) {
        hi = *str++;
        if ((hi >= 0x1 && hi <= 0x13) ||
            (hi >= 0x18 && hi <= 0x1F) ||
            (hi >= 0x28 && hi <= 0x2E))
        {
            matched = TRUE;
            lo = *str++;
            if (lo == 0x50)
                return FALSE;
            charCode = (hi << 8) | lo;
            charCodeAlt = DecodeCrystalCHSChar(charCode);
            result = result && (charCodeAlt != CHAR_QUESTION_MARK || charCode == 0x01F5);
        }
        else if (hi < 0x80)
            return FALSE;
    }
    return matched && result;
}

bool8 ContainsGSCHSChars(const u8 *str) {
    u8 hi, lo;
    u16 charCode, charCodeAlt;
    bool8 result = TRUE;
    bool8 matched = FALSE;
    while (*str != 0x50) {
        hi = *str++;
        if (hi >= 0x1 && hi <= 0xB) {
            matched = TRUE;
            lo = *str++;
            if (lo == 0x50)
                return FALSE;
            charCode = (hi << 8) | lo;
            charCodeAlt = DecodeGSCHSChar(charCode);
            result = result && (charCodeAlt != CHAR_QUESTION_MARK ||
                                (charCode == 0x09DC ||
                                 charCode == 0x0A1F ||
                                 charCode == 0x0A24 ||
                                 charCode == 0x0A67));
        }
        else if (hi < 0x80)
            return FALSE;
    }
    return matched && result;
}

u32 DecodeGSCString(u8 * dest, size_t destLen, const u8 * src, size_t srcLen, u8 gameVer, u8 gameLang)
{
    bool8 isGSKorean = FALSE;
    bool8 isCrystalCHS = FALSE;
    const u8 *table;
    s32 charCode;
    size_t srcPos, destPos;
    u8 hi, lo;
    if ((gameVer == VERSION_GOLD || gameVer == VERSION_SILVER) && gameLang == LANGUAGE_KOREAN)
        isGSKorean = TRUE;
    else if (gameVer == VERSION_CRYSTAL && gameLang == LANGUAGE_CHINESE)
        isCrystalCHS = TRUE;
    table = gameLang == LANGUAGE_JAPANESE ? CharTableJPN : CharTableIntl;
    srcPos = destPos = 0;
    for (; srcPos < srcLen && destPos < destLen - 1 && src[srcPos] != 0x50;)
    {
        if ((isCrystalCHS || isGSKorean) && 
            srcPos + 1 < srcLen && 
            ((lo = src[srcPos + 1]) != 0x50)) {
            charCode = -1;
            if (isCrystalCHS) {
                hi = src[srcPos];
                if ((hi >= 0x1 && hi <= 0x13) ||
                    (hi >= 0x18 && hi <= 0x1F) ||
                    (hi >= 0x28 && hi <= 0x2E))
                {
                    charCode = DecodeCrystalCHSChar((hi << 8) | lo);
                    srcPos += 2;
                }
            }
            if (isGSKorean) {
                hi = src[srcPos];
                if (hi >= 0x1 && hi <= 0xB) {
                    charCode = DecodeGSCHSChar((hi << 8) | lo);
                    srcPos += 2;
                }
            }
            if (charCode >= 0) {
                if (charCode < 256) {
                    dest[destPos++] = charCode & 0xFF;
                }
                else {
                    if (destPos + 2 < destLen) {
                        dest[destPos++] = charCode >> 8;
                        dest[destPos++] = charCode & 0xFF;
                    }
                    else {
                        dest[destPos] = EOS;
                        return destPos;
                    }
                }
                continue;
            }
        }
        dest[destPos++] = table[src[srcPos++]];
    }
    dest[destPos] = EOS;

    if (gameLang == LANGUAGE_JAPANESE)
        ConvertSpecificKana(dest, destPos);
    return destPos;
}

u16 IdentifyInvalidNameChars(u8 *str, size_t maxLen, u8 strLang, const u8 *origStr, u8 origGameVer, u8 origGameLang) {
    bool8 isGSKorean = FALSE;
    bool8 isCrystalCHS = FALSE;
    u16 flag = 0;
    u16 chars[16], origChars[16];
    s32 i, j, left, right;
    u8 hi;
    u16 charCode, origCharCode;
    if ((origGameVer == VERSION_GOLD || origGameVer == VERSION_SILVER) && origGameLang == LANGUAGE_KOREAN)
        isGSKorean = TRUE;
    else if (origGameVer == VERSION_CRYSTAL && origGameLang == LANGUAGE_CHINESE)
        isCrystalCHS = TRUE;
    for (i = 0, j = 0; i < ARRAY_COUNT(origChars) - 1 && origStr[j] != 0x50; i++) {
        if ((isGSKorean || isCrystalCHS) && origStr[j + 1] != 0x50) {
            hi = origStr[j];
            if (isCrystalCHS) {
                if ((hi >= 0x1 && hi <= 0x13) ||
                    (hi >= 0x18 && hi <= 0x1F) ||
                    (hi >= 0x28 && hi <= 0x2E))
                {
                    origChars[i] = (hi << 8) | origStr[j + 1];
                    j += 2;
                    continue;
                }
            }
            if (isGSKorean) {
                if (hi >= 0x1 && hi <= 0xB) {
                    origChars[i] = (hi << 8) | origStr[j + 1];
                    j += 2;
                    continue;    
                }
            }
        }
        origChars[i] = origStr[j++];
    }
    origChars[i] = 0x50;

    for (i = 0, j = 0; i < ARRAY_COUNT(chars) - 1 && str[j] != EOS; i++) {
        if ((isGSKorean || isCrystalCHS) && str[j + 1] != EOS) {
            hi = str[j];
            if ((hi >= 0x01 && hi <= 0x1E) && (hi != 0x06 || hi != 0x1B)) {
                chars[i] = (hi << 8) | str[j + 1];
                j += 2;
                continue;
            }
        }
        chars[i] = str[j++];
    }
    chars[i] = EOS;

    for (i = 0; chars[i] != EOS && origChars[i] != 0x50; i++) {
        charCode = chars[i];
        origCharCode = origChars[i];
        if ((strLang == LANGUAGE_JAPANESE && origGameLang != LANGUAGE_JAPANESE) ||
            (strLang != LANGUAGE_CHINESE && strLang != LANGUAGE_JAPANESE)) {
            for (j = 0; j < ARRAY_COUNT(InvalidNameCharsIntl); j++) {
                if (charCode == InvalidNameCharsIntl[j][0]) {
                    chars[i] = InvalidNameCharsIntl[j][1];
                    flag |= 1 << i;
                    break;
                }
            }
            if (j < ARRAY_COUNT(InvalidNameCharsIntl)) continue;
            if (charCode == CHAR_COMMA) {
                chars[i] = CHAR_BULLET;
                flag |= 1 << i;
                continue;
            }
        }
        if ((isGSKorean || isCrystalCHS)) {
            if (strLang == LANGUAGE_JAPANESE) {
                if (charCode >= 0x100) {
                    chars[i] = CHAR_QUESTION_MARK;
                    flag |= 1 << i;
                    continue;
                }
            }
            for (j = 0; j < ARRAY_COUNT(InvalidNameCharsCHS); j++) {
                if (charCode == InvalidNameCharsCHS[j][0]) {
                    chars[i] = InvalidNameCharsCHS[j][1];
                    flag |= 1 << i;
                    break;
                }
            }
            if (j < ARRAY_COUNT(InvalidNameCharsCHS)) continue;
            if (isGSKorean) {
                if (origCharCode == 0x09DC ||
                    origCharCode == 0x0A1F ||
                    origCharCode == 0x0A24 ||
                    origCharCode == 0x0A67)
                    flag |= 1 << i;
                continue;
            }
            left = 0;
            right = ARRAY_COUNT(CharTableCrystalCHS_Ext) - 1;
            while (left <= right) {
                j = left + (right - left) / 2;
                if (CharTableCrystalCHS_Ext[j].origCharCode == origCharCode) {
                    if (CharTableCrystalCHS_Ext[j].isReplaced) {
                        if (strLang == LANGUAGE_JAPANESE || (origCharCode >= 0x011B && origCharCode <= 0x011E))
                            break;
                        flag |= 1 << i;
                    }
                    break;
                }
                if (CharTableCrystalCHS_Ext[j].origCharCode > origCharCode) {
                    right = j - 1;
                }
                else {
                    left = j + 1;
                }
            }
        }

        if (strLang != LANGUAGE_GERMAN && strLang != LANGUAGE_CHINESE) {
            for (j = 0; j < ARRAY_COUNT(ValidNameCharsGerman); j++) {
                if (charCode == ValidNameCharsGerman[j][0]) {
                    chars[i] = ValidNameCharsGerman[j][1];
                    flag |= 1 << i;
                    break;
                }
            }
        }
    }

    memset(str, 0, maxLen);
    for (i = 0, j = 0; i < maxLen - 1 && chars[j] != EOS; j++)
    {
        charCode = chars[j];
        if (charCode > 0x100) {
            if (i + 2 < maxLen) {
                str[i] = charCode >> 8;
                str[i + 1] = charCode & 0xFF;
                i += 2;
            }
            else {
                break;
            }
        }
        else {
            str[i++] = charCode;
        }
    }
    str[i] = EOS;
    return flag;
}

void HighlightInvalidNameChars(const u8 *src, u8 * dest, u16 nameFlags, u8 strLang) {
    s32 i = 0;
    u8 hi;
    bool8 status = FALSE;
    if (strLang == LANGUAGE_JAPANESE) {
        *dest++ = *src++;
        *dest++ = *src++;
    }
    while (*src != EOS && (strLang != LANGUAGE_JAPANESE || *src != EXT_CTRL_CODE_BEGIN))
    {
        if ((nameFlags & (1 << i)) != 0 && !status) {
            status = !status;
            *dest++ = EXT_CTRL_CODE_BEGIN;
            *dest++ = EXT_CTRL_CODE_COLOR;
            *dest++ = TEXT_COLOR_RED;
        }

        if ((nameFlags & (1 << i)) == 0 && status) {
            status = !status;
            *dest++ = EXT_CTRL_CODE_BEGIN;
            *dest++ = EXT_CTRL_CODE_COLOR;
            *dest++ = TEXT_COLOR_DARK_GRAY;
        }

        hi = *src++;
        if (strLang == LANGUAGE_CHINESE && hi >= 0x01 && hi <= 0x1E && hi != 0x06 && hi != 0x1B) {
            *dest++ = hi;
            *dest++ = *src++;
        }
        else {
            *dest++ = hi;
        }
        i++;
    }
    *dest = EOS;
}

void PaddingName(u8 *name, u8 nameLanguage, u8 nameType) {
    static const u8 nameLengths[][2] = {
        {6, 6},
        {10, 7}
    };
    s32 nameLen = nameLengths[nameLanguage == LANGUAGE_JAPANESE ? 0 : 1][nameType];
    s32 maxLen = nameLengths[1][nameType];
    s32 i = 0;

    for (; name[i] != EOS; i++);

    for (; i < nameLen; i++)
        name[i] = EOS;

    for (; i < maxLen; i++)
        name[i] = 0;
}