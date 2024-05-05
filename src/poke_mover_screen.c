#include "global.h"
#include "bg.h"
#include "cart_detect.h"
#include "data.h"
#include "define_gsc.h"
#include "dma3.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "link.h"
#include "link_poke_mover.h"
#include "load_save.h"
#include "m4a.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "multiboot.h"
#include "overworld.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon_gsc.h"
#include "pokemon_icon.h"
#include "pokemon_rom_resource.h"
#include "pokemon_storage_system.h"
#include "save.h"
#include "scanline_effect.h"
#include "strings.h"
#include "string_util.h"
#include "string_util_gsc.h"
#include "task.h"
#include "text.h"
#include "text_ext.h"
#include "text_window.h"
#include "window.h"
#include "constants/rgb.h"
#include "gba/multiboot_handler.h"

#define tState              data[0]
#define tSubState           data[1]
#define tBoxState           data[2]
#define tLegalityViewState  data[2]
#define tSelectedOption     data[3]
#define tTutorialFrame      data[2]
#define tTutorialCounter    data[3]
#define tTransferingMonNum  data[13]
#define tWindowId           data[14]
#define tPokeStorageOption  data[15]


#define BOX_TITLE_SLOT_0_TILE_OFFSET 0x7B00
#define BOX_TITLE_SLOT_1_TILE_OFFSET 0x7D00
#define GSC_HERO_ICON_TILE_OFFSET 0x7F00
#define TILEMAP_COMMON_BEGIN 0x80
#define TILEMAP_BOXVIEW_GAME_TILE_BEGIN (sWindowTemplates[ARRAY_COUNT(sWindowTemplates) - 1].baseBlock + sWindowTemplates[ARRAY_COUNT(sWindowTemplates) - 1].width * sWindowTemplates[ARRAY_COUNT(sWindowTemplates) - 1].height)
#define TILEMAP_BOXVIEW_LEFT_VIEW_BEGIN (TILEMAP_BOXVIEW_GAME_TILE_BEGIN + 40)
#define TILEMAP_WALLPAPER_BEGIN (TILEMAP_BOXVIEW_LEFT_VIEW_BEGIN + 100)
#define TILEMAP_SCROLL_BG_BEGIN (TILEMAP_WALLPAPER_BEGIN + 256)
#define TILEMAP_LEGALITY_VIEW_BEGIN (sWindowTemplate_LegalityViewHint.baseBlock + sWindowTemplate_LegalityViewHint.width * sWindowTemplate_LegalityViewHint.height)

#define BOTTOM_HINT_WIDTH 30
#define BOTTOM_HINT_HEIGHT 2

#define DecompressAndLoadBgGfxDirectly(src, offset) LZ77UnCompVram(src, (u8*)VRAM + (offset) * TILE_SIZE_4BPP)

struct Wallpaper
{
    const u32 *tiles;
    const u32 *tilemap;
    const u16 *palettes;
};

struct BoxMonGSCBase
{
    u16 species : 8;
    u16 unownLetter : 5; // bit7: item, bit6: egg, bit5-bit0: unown letter
    u16 isShiny : 1;
    u16 isEgg : 1;
    u16 hasItem : 1;
} __attribute__((packed));


struct GSCBaseData {
    u8 gameVer;
    u8 gameLang;
    u8 trainerID[2];
    u8 playerName[max(NAME_LENGTH_GSC_JP, NAME_LENGTH_GSC_INTL) + 1];
    u8 playerGender;
    union
    {
        struct BoxMonGSCBase jp[BOX_TOTAL_GSC_JP][BOX_MON_TOTAL_GSC_JP];
        struct BoxMonGSCBase intl[BOX_TOTAL_GSC_INTL][BOX_MON_TOTAL_GSC_INTL];
    } boxes;
};

struct IncomingSprite {
    const struct SpriteTemplate *template;
    u16 tileNum;
    u16 paletteTag;
    s16 posX;
    s16 posY;
    u8 priority;
    u8 subpriority;
    bool8 isTransparent;
    bool8 requireCopyTile;
    const u8 *tilePtr;
};

struct SpriteManager {
    u8 memflags[0x4000 / (32 * 32 / 2) / 8];
    s8 spriteIds[50];
    struct IncomingSprite * incomingSprites[40];
};

struct BoxDataSource {
    u8 maxRow;
    u8 maxColumn;
    u8 maxBoxCount;
    u8 leftBorder;
    u8 topBorder;
    u8 iconWidth;
    u8 iconHeight;
    u16 (*GetBoxMonIconSpecies)(u8 box, u8 row, u8 column, bool8 * isTransparent);
    void (*GetLocalBoxTitle)(u8 box, u8 *boxTitle);
    const struct Wallpaper * (*GetWallpaper)(u8 box, bool8 * isWaldaWallpaper);
};

struct ScrollableBoxView
{
    u8 ALIGNED(4) bg1Tilemap[256 / 8 * 256 / 8 * 2];
    u8 ALIGNED(4) bg2Tilemap[512 / 8 * 256 / 8 * 2];
    u16 wallpaperTilemap[160 / 8 * 144 / 8];
    u8 state;
    u8 currentBox;
    u8 latestBox;
    u8 gscBox;
    u8 localBox;
    s8 direction;
    u8 slot;
    u8 gscHeroIconSpriteId;
    u16 bg2PosX;
    u8 scrollTimer;
    s8 scrollSpeed;
    u16 gscPlayerNameWinId;
    u16 gscPlayerTrainerIdWinId;
    const struct BoxDataSource *boxDataSource;
};

struct ScrollableLegalityView
{
    u8 state;
    u8 currentIndex;
    u8 latestIndex;
    bool8 isScrollUp;
    u8 bg2PosY;
    u8 scrollTimer;
    s8 scrollSpeed;
    u8 windowIds[4];
};

struct PokeMoverLinkStatus
{
    u8 state;
    u8 status;
    bool8 newDataReached;
    u8 retry;
    u8 currentBox;
    u8 currentOffset;
    u16 timer;
};

struct PokeMoverContext
{
    struct GSCBaseData baseData;
    union 
    {
        struct BoxGSCJP jp;
        struct BoxGSCIntl intl;
    } boxData;
    struct PokeMoverLinkStatus linkStatus;
    struct ScrollableBoxView boxView;
    struct ScrollableLegalityView legalityView;
    struct SpriteManager spriteMgr;
    struct LegalityCheckResult checkResults[30];
    u8 checkResultNum;
    u8 convertedGSCBoxMons[30];
    u8 frameDelay;
    bool8 bg3Scorllable;
};

static u16 GetMonIconSpeciesFromLocalBox(u8 box, u8 row, u8 column, bool8 * isTransparent);
static u16 GetMonIconSpeciesFromGSCBox(u8 box, u8 row, u8 column, bool8 * isTransparent);
static void GetLocalBoxTitle(u8 box, u8 *boxTitle);
static void GetGSCBoxTitle(u8 box, u8 *boxTitle);
static const struct Wallpaper *GetLocalBoxWallpaper(u8 box, bool8 * isWaldaWallpaper);
static const struct Wallpaper *GetGSCBoxWallpaper(u8 box, bool8 * isWaldaWallpaper);
static void Task_HandlePokeMoverMenu(u8 taskId);
static void Task_SetupPokeMover(u8 taskId);
static void CB2_PokeMover(void);
static void CB2_ExitPokeMover(void);
static void VBlankCB_PokeMover(void);
static u32 CalcGSCTransferableBoxMonCount(u8 box);
static bool8 CheckLocalBoxHasEnoughSpace();
static s32 DoGSCBoxMonConversion(u8 localBox, struct BoxMonGSCBase *bases, u8 *convertedBoxMons);
static void UpdatePokedex();

extern const u32 sScrollingBg_Gfx[];
extern const u32 sScrollingBg_Tilemap[];
extern const u16 sScrollingBg_Pal[];

extern const struct SpriteTemplate sSpriteTemplate_MonIcon;
extern const struct SpriteTemplate sSpriteTemplate_BoxTitle;

static struct PokeMoverContext *sPokeMoverContext;

static const u8 gMsgWelcomeForPokeMover[] = _("欢迎使用Pokémover For GBA。\p"
                                              "你可以使用本工具\n"
                                              "将“宝可梦 金/银/水晶版”中的宝可梦\l"
                                              "传输到盒子里。\p");
static const u8 gMsgInsertCart[] = _("请插入GBA宝可梦系列的游戏卡带，\n"
                                     "插入卡带后按A键继续。\p");
static const u8 gTextDetectCart[] = _("正在检查游戏卡带，请稍候……");
static const u8 gTextInvaildCart[] = _("游戏卡带不符合要求，\n"
                                       "请检查后重试。");
static const u8 gTextInvaildSave[] = _("无法从游戏卡带中读取存档，\n"
                                       "请检查后重试。");
static const u8 gMsgAskForNext[] = _("请问你要做什么？");
static const u8 gTextSendingTransferToolOption[] = _("发送传输工具");
static const u8 gTextStartTransferOption[] = _("开始传输");
static const u8 gTextInfoOption[] = _("听取说明");
static const u8 gTextExitOption[] = _("退出");
static const u8 gMsgAskForConsole[] = _("请选择要发送的目标游戏主机。");
static const u8 gMsgUsageOfSendingTransferTool[] = _("开始发送前，请准备好另一台没有打开\n"
                                                     "电源、没有插入任何游戏卡带的GBA。\p"
                                                     "用GB(C)连接线连接2台GBA，\n"
                                                     "然后打开另外一台GBA的电源，\l"
                                                     "准备完成后，请按下A键。\p");

static const u8 gMsgUsageStep1OfSendingTransferToolOverExploit[] = _("请准备一张“宝可梦 水晶版”的游戏卡带，\n"
                                                                     "将使用该游戏中的漏洞发送传输工具，\p"
                                                                     "请问你准备的卡带\n"
                                                                     "是日语版还是国际版？");

static const u8 gMsgUsageStep2OfSendingTransferToolOverExploit[] = _("请将“宝可梦 水晶版”的游戏卡带\n"
                                                                     "插入GBC，接着启动游戏，\l"
                                                                     "来到宝可梦中心的连接交换柜台前，\l"
                                                                     "然后用GB(C)连接线连接GBA跟GBC。\p"
                                                                     "连接时请注意先后顺序，\n"
                                                                     "先由GBA一方发起连接，\p"
                                                                     "在GBA一方发起连接后，GBC一方\n"
                                                                     "再跟柜台的工作人员对话进行连接。\p"
                                                                     "准备完成后，\n"
                                                                     "按下本机的A键发起连接。\p");
                                                                     
static const u8 gMsgUsageStep3OfSendingTransferToolOverExploit[] = _("接下来将使用漏洞发送传输工具，\n"
                                                                     "为了确保正确触发漏洞，\l"
                                                                     "请之后务必按照指定的步骤操作。\p");
                                                                     
static const u8 gMsgUsageStep4OfSendingTransferToolOverExploit[] = _("如图所示，请先移动到椅子的正下方，\n"
                                                                     "然后，打开菜单，接着关闭菜单，\p"
                                                                     "关闭菜单后，再向上走一步，\n"
                                                                     "坐到椅子上之后，按A键进行连接。\p"
                                                                     "准备完成后，\n"
                                                                     "按下本机的A键继续。\p");


static const u8 gTextGBAOption[] = _("发送至GBA");
static const u8 gTextGBCOption[] = _("发送至GBC");

static const u8 gTextJPNOption[] = _("日语版");
static const u8 gTextINTLOption[] = _("国际版");
static const u8 gTextReturnOption[] = _("返回");

static const u8 gTextSending[] = _("发送中……\n"
                                   "请勿关闭电源或拔出游戏卡带。");
static const u8 gTextSendUnsuccessfully[] = _("发送失败，\n"
                                              "请检查连接是否正确。");
static const u8 gTextLinkToGSC[] = _("正在与“宝可梦 水晶版”建立连接，\n"
                                     "请与连接交换柜台的工作人员对话。");
static const u8 gTextSendToGBCUnsuccessfully0[] = _("发送失败，\n"
                                                    "没能进入连接交换的房间。");
static const u8 gTextSendToGBCUnsuccessfully1[] = _("发送失败，没能触发漏洞，请检查步骤\n"
                                                    "以及GBC一端的游戏版本跟游戏语言。");
static const u8 gTextSendSuccessfully[] = _("发送成功，接下来\n"
                                            "可选择菜单中的“开始传输”进行传输。");
static const u8 gTextUsageOfTranfering[] = _("在开始传输前，请确保已经将\n"
                                             "传输工具发送至另外一台游戏主机上，\p"
                                             "并且使用GB(C)连接线\n"
                                             "连接2台游戏主机。\p"
                                             "然后按照传输工具的提示，插入\n"
                                             "“宝可梦 金/银/水晶版”的游戏卡带。\p"
                                             "在插入游戏卡带之后，\n"
                                             "再根据提示进行操作，\l"
                                             "准备完成后，请按下A键开始传输。\p");
static const u8 gTextLinking[] = _("传输中……\n"
                                   "请勿关闭电源或拔出游戏卡带。");
static const u8 gTextRecvUnsuccessfully[] = _("传输失败，\n"
                                              "请检查连接是否正确。");
static const u8 gTextRecvSuccessfully[] = _("传输完成！");
static const u8 gTextConfirmTransferingBox[] = _("确定要传输{STR_VAR_1}吗？");
static const u8 gTextConfirmReceivingBox[] = _("确定要用{STR_VAR_1}接收吗？");
static const u8 gTextNoTransferableMon[] = _("没有可传输的宝可梦。");
static const u8 gTextNoEnoughSpace[] = _("没有多余的空间可以接收。");
static const u8 gTextProcessing[] = _("正在处理中……\n"
                                      "请勿关闭电源或拔出游戏卡带。");
static const u8 gTextUsageOfLegalityView[] = _("注意！在传输完成后，\n"
                                               "某些宝可梦的等级将会提升，\l"
                                               "并且一部分招式可能被遗忘。\p"
                                               "请确认之后，再决定是否进行传输。\n"
                                               "(变动的地方将以{COLOR RED}红字{COLOR DARK_GRAY}标出)\p");
static const u8 gTextSaving[] = _("正在写入记录……\n"
                                  "请勿关闭电源或拔出游戏卡带。");
static const u8 gTextSaveUnsuccessfully[] = _("发生错误，即将重启……");
static const u8 gTextSaveSuccessfully[] = _("已写下记录！\n");

static const u8 gTextAskForInfo[] = _("想要了解哪方面的问题？");

static const u8 gTextInfoQ1[] = _("准备工作有哪些？");
static const u8 gTextInfoQ2[] = _("如何进行传输？");
static const u8 gTextInfoQ3[] = _("有哪些宝可梦不可传输");
static const u8 gTextInfoQ4[] = _("昵称有何变化？");

static const u8 gTextInfoA1[] = _("游戏主机方面，除了本机以外，\n"
                                  "还需要再准备一台GBA(SP)或者GBC。\p"
                                  "对于使用GBC进行连接的玩家，\n"
                                  "还需要准备一张“宝可梦 水晶版”的\l"
                                  "游戏卡带。\p"
                                  "此外，请注意，\n"
                                  "使用GB(C)的连接线才能确保连接，\l"
                                  "GBA的连接线不可用。\p");

static const u8 gTextInfoA2[] = _("先选择菜单中的“发送传输工具”选项，\n"
                                  "按照步骤发送传输工具到目标主机，\l"
                                  "然后根据传输工具的提示插入卡带。\p"
                                  "直到传输工具出现\n"
                                  "“等待连接中”的提示时，就可以\l"
                                  "选择“开始传输”选项进行传输了。\p"
                                  "进行传输的时候，先选择\n"
                                  "要传输的盒子，再选择要接收的盒子。\p"
                                  "传输过程中，异色宝可梦\n"
                                  "可能要花比较久的时间处理，\l"
                                  "请耐心等待。\p"
                                  "当传输完成后，某些宝可梦的等级\n"
                                  "会提升，部分招式可能会被遗忘掉，\l"
                                  "请确认过这些变化后，\l"
                                  "再确定是否进行接收。\p"
                                  "如果确定接收，将写入记录，\n"
                                  "写入记录的时候，\l"
                                  "请确保2台主机始终使用连接线连接，\p"
                                  "并且不要切断任意一方的电源或\n"
                                  "拔出游戏卡带。\p");

static const u8 gTextInfoA3[] = _("不可传输的宝可梦包括: \n"
                                  "携带道具的宝可梦、蛋、学会了\l"
                                  "任意秘传学习器招式的宝可梦，\l"
                                  "以及异色时拉比。\p"
                                  "在传输的时候，这些宝可梦的图标\n"
                                  "将显示为半透明。\p");

static const u8 gTextInfoA4[] = _("对于宝可梦的昵称以及初训家名，\n"
                                  "因为GBA的宝可梦系列\l"
                                  "所能使用的取名字符跟\l"
                                  "“金/银/水晶版”不完全一致，\p"
                                  "无法在GBA的宝可梦系列中输入的\n"
                                  "取名字符将转换成近似的字符或问号。\p"
                                  "对于梦幻，名字仅允许使用\n"
                                  "日语版中的取名字符，对于长度超过\l"
                                  "5个字符的名字也将被截断。\p"
                                  "对于时拉比，昵称以及初训家名\n"
                                  "将改成跟日语版圆形竞技场的\l"
                                  "时拉比一致。\p");

const u32 gMultiBootProgram_GSC_Transfer_Tool[] = INCBIN_U32("data/mb_GSC_transfer_tool.gba.lz");

static const struct MenuAction sPokeMoverStartMenuItems[] =
{
    {gTextSendingTransferToolOption},
    {gTextStartTransferOption},
    {gTextInfoOption},
    {gTextExitOption},
};

static const struct MenuAction sPokeMoverLanguageOptions[] =
{
    {gTextJPNOption},
    {gTextINTLOption}
};

static const struct MenuAction sPokeMoverConsoleOptions[] =
{
    {gTextGBAOption},
    {gTextGBCOption},
    {gTextReturnOption}
};

static const struct MenuAction sPokeMoverInfoOptions[] =
{
    {gTextInfoQ1},
    {gTextInfoQ2},
    {gTextInfoQ3},
    {gTextInfoQ4},
    {gTextReturnOption}
};

enum
{
    PALTAG_GSCHEROICON = 65000,
    PALTAG_BOXTITLE
};

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 10,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 11,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 12,
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 14,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    },
};
static const struct WindowTemplate sWindowTemplates[] =
{
    {
        .bg = 1,
        .tilemapLeft = 0,
        .tilemapTop = 18,
        .width = 30,
        .height = 2,
        .paletteNum = 13,
        .baseBlock = TILEMAP_COMMON_BEGIN,
    },
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 8,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = TILEMAP_COMMON_BEGIN + 60,
    },
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 10,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = TILEMAP_COMMON_BEGIN + 76,
    },
    {
        .bg = 0,
        .tilemapLeft = 24,
        .tilemapTop = 9,
        .width = 5,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = TILEMAP_COMMON_BEGIN + 92,
    }
};

static const struct WindowTemplate sWindowTemplate_PokeMoverStartMenu =
{
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 20 - ARRAY_COUNT(sPokeMoverStartMenuItems) * 2 - 7,
    .width = 0,
    .height = ARRAY_COUNT(sPokeMoverStartMenuItems) * 2,
    .paletteNum = 15,
    .baseBlock = TILEMAP_COMMON_BEGIN,
};

static const struct WindowTemplate sWindowTemplate_PokeMoverLanguageOptions =
{
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 20 - ARRAY_COUNT(sPokeMoverLanguageOptions) * 2 - 7,
    .width = 0,
    .height = ARRAY_COUNT(sPokeMoverLanguageOptions) * 2,
    .paletteNum = 15,
    .baseBlock = TILEMAP_COMMON_BEGIN,
};

static const struct WindowTemplate sWindowTemplate_PokeMoverConsoleOptions =
{
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 20 - ARRAY_COUNT(sPokeMoverConsoleOptions) * 2 - 7,
    .width = 0,
    .height = ARRAY_COUNT(sPokeMoverConsoleOptions) * 2,
    .paletteNum = 15,
    .baseBlock = TILEMAP_COMMON_BEGIN,
};

static const struct WindowTemplate sWindowTemplate_PokeMoverInfoOptions =
{
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 20 - ARRAY_COUNT(sPokeMoverInfoOptions) * 2 - 7,
    .width = 0,
    .height = ARRAY_COUNT(sPokeMoverInfoOptions) * 2,
    .paletteNum = 15,
    .baseBlock = TILEMAP_COMMON_BEGIN,
};

static const struct WindowTemplate sWindowTemplate_LegalityViewHint = {
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 18,
    .width = 30,
    .height = 2,
    .paletteNum = 13,
    .baseBlock = TILEMAP_COMMON_BEGIN
};

static const struct WindowTemplate sWindowTemplates_LegalityView[] = 
{
    {
        .bg = 2,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 28,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = TILEMAP_LEGALITY_VIEW_BEGIN
    },
    {
        .bg = 2,
        .tilemapLeft = 1,
        .tilemapTop = 7,
        .width = 28,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = TILEMAP_LEGALITY_VIEW_BEGIN + 28 * 4
    },
    {
        .bg = 2,
        .tilemapLeft = 1,
        .tilemapTop = 13,
        .width = 28,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = TILEMAP_LEGALITY_VIEW_BEGIN + 28 * 4 * 2
    },
    {
        .bg = 2,
        .tilemapLeft = 1,
        .tilemapTop = 19,
        .width = 28,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = TILEMAP_LEGALITY_VIEW_BEGIN + 28 * 4 * 3
    }
};

static const struct OamData sOamData_GSCHeroIcon;
const struct SpriteTemplate sSpriteTemplate_GSCHeroIcon =
{
    .tileTag = 0,
    .paletteTag = PALTAG_GSCHEROICON,
    .oam = &sOamData_GSCHeroIcon,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct OamData sOamData_GSCHeroIcon =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0
};

static struct Wallpaper sWallpaper;

static const struct BoxDataSource localBoxDataSource = {
    .maxRow = 5,
    .maxColumn = 6,
    .maxBoxCount = TOTAL_BOXES_COUNT,
    .leftBorder = 0,
    .topBorder = 0,
    .iconWidth = 24,
    .iconHeight = 24,
    .GetBoxMonIconSpecies = GetMonIconSpeciesFromLocalBox,
    .GetLocalBoxTitle = GetLocalBoxTitle,
    .GetWallpaper = GetLocalBoxWallpaper
};

static const struct BoxDataSource gscJPNBoxDataSource = {
    .maxRow = 5,
    .maxColumn = 6,
    .maxBoxCount = BOX_TOTAL_GSC_JP,
    .leftBorder = 0,
    .topBorder = 0,
    .iconWidth = 24,
    .iconHeight = 24,
    .GetBoxMonIconSpecies = GetMonIconSpeciesFromGSCBox,
    .GetLocalBoxTitle = GetGSCBoxTitle,
    .GetWallpaper = GetGSCBoxWallpaper
};

static const struct BoxDataSource gscINTLBoxDataSource = {
    .maxRow = 4,
    .maxColumn = 5,
    .maxBoxCount = BOX_TOTAL_GSC_INTL,
    .leftBorder = 2,
    .topBorder = 0,
    .iconWidth = 28,
    .iconHeight = 30,
    .GetBoxMonIconSpecies = GetMonIconSpeciesFromGSCBox,
    .GetLocalBoxTitle = GetGSCBoxTitle,
    .GetWallpaper = GetGSCBoxWallpaper
};


static const u32 gscHeroIconSprite_Gfx[] = INCBIN_U32("graphics/poke_mover/GSC_hero_icon.4bpp");
static const u16 gscHeroIconSprite_Pal[16] = INCBIN_U16("graphics/poke_mover/GSC_hero_icon.gbapal");

static const u32 crystalExploitTotorial_Gfx[] = INCBIN_U32("graphics/poke_mover/crystal_exploit_tutorial.4bpp.lz");
static const u32 crystalExploitTotorial_Tilemap[] = INCBIN_U32("graphics/poke_mover/crystal_exploit_tutorial.bin");
static const u16 crystalExploitTotorial_Pal[16] = INCBIN_U16("graphics/poke_mover/crystal_exploit_tutorial.gbapal");

static const u32 boxViewPokeGoldTitle_Gfx[] = INCBIN_U32("graphics/poke_mover/poke_gold_title.bin.lz");
static const u32 boxViewPokeGoldTitle_Tilemap[] = INCBIN_U32("graphics/poke_mover/poke_gold_title_tilemap.bin");
static const u16 boxViewPokeGoldTitle_Pal[] = INCBIN_U16("graphics/poke_mover/poke_gold_title_pal.bin");

static const u32 boxViewPokeSliverTitle_Gfx[] = INCBIN_U32("graphics/poke_mover/poke_sliver_title.bin.lz");
static const u32 boxViewPokeSliverTitle_Tilemap[] = INCBIN_U32("graphics/poke_mover/poke_sliver_title_tilemap.bin");
static const u16 boxViewPokeSliverTitle_Pal[] = INCBIN_U16("graphics/poke_mover/poke_sliver_title_pal.bin");

static const u32 boxViewPokeCrystalTitle_Gfx[] = INCBIN_U32("graphics/poke_mover/poke_crystal_title.bin.lz");
static const u32 boxViewPokeCrystalTitle_Tilemap[] = INCBIN_U32("graphics/poke_mover/poke_crystal_title_tilemap.bin");
static const u16 boxViewPokeCrystalTitle_Pal[] = INCBIN_U16("graphics/poke_mover/poke_crystal_title_pal.bin");

static const u32 boxViewGBSkin_Gfx[] = INCBIN_U32("graphics/poke_mover/box_view_gb_skin.bin.lz");
static const u32 boxViewGBSkin_Tilemap[] = INCBIN_U32("graphics/poke_mover/box_view_gb_skin_tilemap.bin");
static const u16 boxViewGBSkin_Pal[] = INCBIN_U16("graphics/poke_mover/box_view_gb_skin_pal.bin");

static const u32 boxViewGBCSkin_Gfx[] = INCBIN_U32("graphics/poke_mover/box_view_gbc_skin.bin.lz");
static const u32 boxViewGBCSkin_Tilemap[] = INCBIN_U32("graphics/poke_mover/box_view_gbc_skin_tilemap.bin");
static const u16 boxViewGBCSkin_Pal[] = INCBIN_U16("graphics/poke_mover/box_view_gbc_skin_pal.bin");

static const u32 boxViewBottomBar0_Gfx[] = INCBIN_U32("graphics/poke_mover/bottom_bar_0.4bpp.lz");
static const u16 boxViewBottomBar0_Pal[16] = INCBIN_U16("graphics/poke_mover/bottom_bar_0.gbapal");

static const u32 boxViewBottomBar1_Gfx[] = INCBIN_U32("graphics/poke_mover/bottom_bar_1.4bpp.lz");
static const u16 boxViewBottomBar1_Pal[16] = INCBIN_U16("graphics/poke_mover/bottom_bar_1.gbapal");

static const u32 boxViewBottomBar2_Gfx[] = INCBIN_U32("graphics/poke_mover/bottom_bar_2.4bpp.lz");
static const u16 boxViewBottomBar2_Pal[16] = INCBIN_U16("graphics/poke_mover/bottom_bar_2.gbapal");

#define GET_NAME_LENGTH (sPokeMoverContext->baseData.gameLang == LANGUAGE_JAPANESE ? NAME_LENGTH_GSC_JP : NAME_LENGTH_GSC_INTL)
#define GET_BOX_MON_TOTAL (sPokeMoverContext->baseData.gameLang == LANGUAGE_JAPANESE ? BOX_MON_TOTAL_GSC_JP : BOX_MON_TOTAL_GSC_INTL)
#define GET_BOX_SIZE (sPokeMoverContext->baseData.gameLang == LANGUAGE_JAPANESE ? sizeof(struct BoxGSCJP) : sizeof(struct BoxGSCIntl))

static void InitSpritePalette() {
    struct SpritePalette gscHeroSprPal = {
        .data = gscHeroIconSprite_Pal,
        .tag = PALTAG_GSCHEROICON
    };
    u16 boxTitlePalette[16] = {0};
    struct SpritePalette boxTitleSprPal = {
        .tag = PALTAG_BOXTITLE};
    boxTitlePalette[14] = RGB(7, 7, 7);
    boxTitlePalette[15] = RGB_WHITE;
    LoadSpritePalette(&gscHeroSprPal);
    boxTitleSprPal.data = boxTitlePalette;
    LoadSpritePalette(&boxTitleSprPal);
}

static void InitSpriteMgr() {
    int i;
    memset(&sPokeMoverContext->spriteMgr.memflags, 0, sizeof(sPokeMoverContext->spriteMgr.memflags));
    for (i = 0; i < ARRAY_COUNT(sPokeMoverContext->spriteMgr.spriteIds); i++) {
        if (sPokeMoverContext->spriteMgr.spriteIds[i] >= 0) {
            DestroySprite(&gSprites[sPokeMoverContext->spriteMgr.spriteIds[i]]);
        }
        sPokeMoverContext->spriteMgr.spriteIds[i] = -1;
    }
    for (i = 0; i < ARRAY_COUNT(sPokeMoverContext->spriteMgr.incomingSprites); i++) {
        if (sPokeMoverContext->spriteMgr.incomingSprites[i] != NULL)
            FREE_AND_SET_NULL(sPokeMoverContext->spriteMgr.incomingSprites[i]);
    }
}


static s32 AllocSpriteTileOffset() {
    int i, j;
    for (i = 0; i < sizeof(sPokeMoverContext->spriteMgr.memflags); i++) {
        for (j = 0; j < 8; j++) {
            if (((sPokeMoverContext->spriteMgr.memflags[i] >> j) & 1) == 0) {
                sPokeMoverContext->spriteMgr.memflags[i] |= (1 << j);
                return (i * 8 + j) * (32 / 8 * 32 / 8) + 0x3B00 / TILE_SIZE_4BPP;
            }
        }
    }
    return -1;
}

static void FreeSpriteTilemem(u32 tileOffset) {
    if (tileOffset >= 0x3B00 / TILE_SIZE_4BPP && tileOffset < BOX_TITLE_SLOT_0_TILE_OFFSET / TILE_SIZE_4BPP) {
        tileOffset -= 0x3B00 / TILE_SIZE_4BPP;
        sPokeMoverContext->spriteMgr.memflags[(tileOffset / ((32 / 8) * (32 / 8))) >> 3] &= ~(1 << ((tileOffset / ((32 / 8) * (32 / 8))) & 7));
    }
}

static void OnSpriteCreated(u8 spriteId) {
    int i;
    if (spriteId >= MAX_SPRITES)
        return;
    for (i = 0; i < sizeof(sPokeMoverContext->spriteMgr.spriteIds); i++) {
        if (sPokeMoverContext->spriteMgr.spriteIds[i] == -1) {
            sPokeMoverContext->spriteMgr.spriteIds[i] = spriteId;
            break;
        }
    }
}

static void OnSpriteRemoved(u8 spriteId) {
    int i;
    if (spriteId >= MAX_SPRITES)
        return;
    for (i = 0; i < sizeof(sPokeMoverContext->spriteMgr.spriteIds); i++) {
        if (sPokeMoverContext->spriteMgr.spriteIds[i] == spriteId) {
            sPokeMoverContext->spriteMgr.spriteIds[i] = -1;
            break;
        }
    }
}

static void CreateSpriteFromIncoming(struct IncomingSprite * incomingSprite) {
    u8 spriteId;
    struct SpriteTemplate template = *incomingSprite->template;
    s32 tileOffset;
    template.paletteTag = incomingSprite->paletteTag;
    template.anims = gDummySpriteAnimTable;
    spriteId = CreateSprite(&template, incomingSprite->posX, incomingSprite->posY, incomingSprite->subpriority);
    if (spriteId >= MAX_SPRITES)
        return;
    gSprites[spriteId].oam.tileNum = incomingSprite->tileNum;
    gSprites[spriteId].oam.priority = incomingSprite->priority;
    if (incomingSprite->isTransparent)
        gSprites[spriteId].oam.objMode = ST_OAM_OBJ_BLEND;
    if (incomingSprite->requireCopyTile) {
        tileOffset = AllocSpriteTileOffset();
        if (tileOffset >= 0) {
            RequestDma3Copy(incomingSprite->tilePtr, (void*)((OBJ_VRAM0) + tileOffset * TILE_SIZE_4BPP), 0x200, 0);
            gSprites[spriteId].oam.tileNum = tileOffset;
        }
    }
    OnSpriteCreated(spriteId);
}

static bool8 InitBGs(struct Task * task)
{
    switch(task->tSubState) {
        case 0:
            InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
            SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_ALL);
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(7, 11));
            SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
            task->tSubState++;
            break;
        case 1:
            DecompressAndLoadBgGfxDirectly(sScrollingBg_Gfx, TILEMAP_SCROLL_BG_BEGIN);
            LZ77UnCompVram(sScrollingBg_Tilemap, (void *)BG_SCREEN_ADDR(14));
            task->tSubState++;
            break;
        case 2:
            if (!FreeTempTileDataBuffersIfPossible()) 
                task->tSubState++;
            break;
        case 3:
            ShowBg(0);
            ShowBg(3);
            sPokeMoverContext->bg3Scorllable = TRUE;
            return TRUE;
    }
    return FALSE;
}

static bool8 InitScrollableBoxView() {
    switch (sPokeMoverContext->boxView.state)
    {
    case 0:
        CpuFastFill(0, sPokeMoverContext->boxView.bg1Tilemap, sizeof(sPokeMoverContext->boxView.bg1Tilemap));
        CpuFastFill(0, sPokeMoverContext->boxView.bg2Tilemap, sizeof(sPokeMoverContext->boxView.bg2Tilemap));
        sPokeMoverContext->boxView.state++;
        break;
    case 1:
        SetBgTilemapBuffer(1, sPokeMoverContext->boxView.bg1Tilemap);
        SetBgTilemapBuffer(2, sPokeMoverContext->boxView.bg2Tilemap);
        sPokeMoverContext->boxView.state++;
        break;
    case 2:
        InitSpritePalette();
        sPokeMoverContext->boxView.state++;
        break;
    case 3:
        ShowBg(2);
        sPokeMoverContext->boxView.state = 0;
        return TRUE;
    }
    return FALSE;
}

static void DrawStandardTextBox() {
    // setup textbox tilemap
    DrawDialogueFrame(0, FALSE);
    // clear textbox tile
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    CopyWindowToVram(0, COPYWIN_FULL);
    ScheduleBgCopyTilemapToVram(0);
}

static void InitTextView() {
    // setup a window
    InitStandardTextBoxWindows();
    // setup textbox tile
    InitTextBoxGfxAndPrinters();

    DrawStandardTextBox();

    sPokeMoverContext->boxView.gscPlayerNameWinId = AddWindow(&sWindowTemplates[1]);
    sPokeMoverContext->boxView.gscPlayerTrainerIdWinId = AddWindow(&sWindowTemplates[2]);
}

static void DrawStartMenu(u8 cursorPos, s16 *windowIdPtr)
{
    s16 windowId;
    struct WindowTemplate template = sWindowTemplate_PokeMoverStartMenu;
    template.width = GetMaxWidthInMenuTable(sPokeMoverStartMenuItems, ARRAY_COUNT(sPokeMoverStartMenuItems));
    template.tilemapLeft = 30 - template.width - 1;
    windowId = AddWindow(&template);

    DrawStdWindowFrame(windowId, FALSE);
    PrintMenuTable(windowId, ARRAY_COUNT(sPokeMoverStartMenuItems), sPokeMoverStartMenuItems);
    InitMenuNormal(windowId, FONT_NORMAL, 0, 0, 16, ARRAY_COUNT(sPokeMoverStartMenuItems), cursorPos);
    *windowIdPtr = windowId;
}

static void DrawOptions(const struct MenuAction *menuAction, u8 numOptions, const struct WindowTemplate *template, s16 *windowIdPtr) {
    s16 windowId;
    struct WindowTemplate templateAlt = *template;
    templateAlt.width = GetMaxWidthInMenuTable(menuAction, numOptions);
    templateAlt.tilemapLeft = 30 - templateAlt.width - 1;
    windowId = AddWindow(&templateAlt);

    DrawStdWindowFrame(windowId, FALSE);
    PrintMenuTable(windowId, numOptions, menuAction);
    InitMenuNormal(windowId, FONT_NORMAL, 0, 0, 16, numOptions, 0);
    *windowIdPtr = windowId;
}

static void DrawDelayedMessage(u8 windowId, const u8 *message, u8 delayedFrames) {
    FillWindowPixelBuffer(0, PIXEL_FILL(1));
    DrawText(0, message, 0, 0, NULL, TRUE);
    sPokeMoverContext->frameDelay = delayedFrames;
}

static bool8 HandleDelayedMessage(u16 keyMasks) {
    return --sPokeMoverContext->frameDelay == 0 || JOY_NEW(keyMasks);
}

static u16 GetMonIconSpeciesFromLocalBox(u8 box, u8 row, u8 column, bool8 * isTransparent) {
    struct BoxPokemon * mon = &gPokemonStoragePtr->boxes[box][row * 6 + column];
    u32 personality = GetBoxMonData(mon, MON_DATA_PERSONALITY, NULL);
    u16 species = GetBoxMonData(mon, MON_DATA_SPECIES_OR_EGG, NULL);
    *isTransparent = FALSE;
    return GetIconSpecies(species, personality);
}

static u16 GetMonIconSpeciesFromGSCBox(u8 box, u8 row, u8 column, bool8 * isTransparent) {
    struct BoxMonGSCBase base;
    if (sPokeMoverContext->baseData.gameLang == LANGUAGE_JAPANESE)
        base = sPokeMoverContext->baseData.boxes.jp[box][row * sPokeMoverContext->boxView.boxDataSource->maxColumn + column];
    else
        base = sPokeMoverContext->baseData.boxes.intl[box][row * sPokeMoverContext->boxView.boxDataSource->maxColumn + column];
    *isTransparent = base.species > SPECIES_CELEBI || base.isEgg || base.hasItem || (base.species == SPECIES_CELEBI && base.isShiny);
    if (base.isEgg)
        return SPECIES_EGG;
    if (base.species == SPECIES_UNOWN)
    {
        if (base.unownLetter == 0)
            return SPECIES_UNOWN;
        else
            return SPECIES_UNOWN_B + base.unownLetter - 1;
    }
    return base.species;
}

static void GetLocalBoxTitle(u8 box, u8 * boxTitle) {
    u8 *tail; 
    if (gRomHeader->language == LANGUAGE_JAPANESE) {
        boxTitle[0] = EXT_CTRL_CODE_BEGIN;
        boxTitle[1] = EXT_CTRL_CODE_JPN;
        tail = StringCopy(&boxTitle[2], gPokemonStoragePtr->boxNames[box]);
        tail[0] = EXT_CTRL_CODE_BEGIN;
        tail[1] = EXT_CTRL_CODE_ENG;
        tail[2] = EOS;
    }
    else {
        StringCopyN(boxTitle, gPokemonStoragePtr->boxNames[box], BOX_NAME_LENGTH + 1);
    }
}

static void GetGSCBoxTitle(u8 box, u8 * boxTitle) {
    u8 *dest = StringCopy(boxTitle, gText_Box);
    ConvertIntToDecimalStringN(dest, box + 1, STR_CONV_MODE_LEFT_ALIGN, 2);
}

static const struct Wallpaper * GetLocalBoxWallpaper(u8 box, bool8 * isWaldaWallpaper) {
    u8 wallPaperId = gPokemonStoragePtr->boxWallpapers[box];
    u8 waldaPatternId = GetWaldaWallpaperPatternId();
    if (wallPaperId != 16)
    {
        *isWaldaWallpaper = FALSE;
        sWallpaper.tiles = GetWallpaperTiles(wallPaperId, FALSE);
        sWallpaper.tilemap = GetWallpaperTilemap(wallPaperId, FALSE);
        sWallpaper.palettes = GetWallpaperPalette(wallPaperId, FALSE);
        return &sWallpaper;
    }
    else {
        *isWaldaWallpaper = TRUE;
        sWallpaper.tiles = GetWallpaperTiles(waldaPatternId, TRUE);
        sWallpaper.tilemap = GetWallpaperTilemap(waldaPatternId, TRUE);
        sWallpaper.palettes = GetWallpaperPalette(waldaPatternId, TRUE);
        return &sWallpaper;
    }
}

static const struct Wallpaper * GetGSCBoxWallpaper(u8 box, bool8 * isWaldaWallpaper) {
    *isWaldaWallpaper = FALSE;
    sWallpaper.tiles = GetWallpaperTiles(box, FALSE);
    sWallpaper.tilemap = GetWallpaperTilemap(box, FALSE);
    sWallpaper.palettes = GetWallpaperPalette(box, FALSE);
    return &sWallpaper;
}

static void CreateIncomingBoxMonIcon(u8 box, int direction) {
    u16 iconSpecies;
    s32 i, j, k;
    const struct BoxDataSource *dataSource = sPokeMoverContext->boxView.boxDataSource;
    bool8 isTransparent;
    struct IncomingSprite * incomingSpr;

    for (i = 0; i < dataSource->maxRow; i++){
        for (j = 0; j < dataSource->maxColumn; j++) {
            for (k = 0; k < ARRAY_COUNT(sPokeMoverContext->spriteMgr.incomingSprites);k++) {
                if (sPokeMoverContext->spriteMgr.incomingSprites[k] == NULL) {
                    isTransparent = FALSE;
                    iconSpecies = dataSource->GetBoxMonIconSpecies(box, i, j, &isTransparent);
                    if (iconSpecies == 0)
                        break;
                    incomingSpr = (struct IncomingSprite *)AllocZeroed(sizeof(struct IncomingSprite));
                    incomingSpr->priority = 2;
                    incomingSpr->subpriority = 19 - j;
                    incomingSpr->paletteTag = sSpriteTemplate_MonIcon.paletteTag + gRomHeader->monIconPaletteIds[iconSpecies];
                    incomingSpr->posX = 192 * direction + 100 + dataSource->leftBorder + dataSource->iconWidth * j;
                    incomingSpr->posY = 28 + dataSource->topBorder + dataSource->iconHeight * i;
                    incomingSpr->template = &sSpriteTemplate_MonIcon;
                    incomingSpr->isTransparent = isTransparent;
                    incomingSpr->requireCopyTile = TRUE;
                    incomingSpr->tilePtr = GetMonIconTiles(iconSpecies, TRUE);
                    sPokeMoverContext->spriteMgr.incomingSprites[k] = incomingSpr;
                    break;
                }
            }
        }
    }
}

static void CreateIncomingBoxTitle(int direction) {
    u8 box, boxTitle[16], *boxTitleTile, x;
    u32 tileOffset;
    int i, j;
    const struct BoxDataSource *dataSource = sPokeMoverContext->boxView.boxDataSource;
    box = sPokeMoverContext->boxView.currentBox;
    dataSource->GetLocalBoxTitle(box, boxTitle);
    boxTitleTile = AllocZeroed(64 * 16 / 2);
    DrawTextWindowAndBufferTiles(boxTitle, boxTitleTile, 0, 0, 2);
    tileOffset = sPokeMoverContext->boxView.slot == 0 ? BOX_TITLE_SLOT_0_TILE_OFFSET : BOX_TITLE_SLOT_1_TILE_OFFSET;
    CpuCopy16(boxTitleTile, (void *)(OBJ_VRAM0 + tileOffset), 512);
    x = DISPLAY_WIDTH - 64 - GetStringWidth(FONT_NORMAL, boxTitle, 0) / 2;
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < ARRAY_COUNT(sPokeMoverContext->spriteMgr.incomingSprites); j++)
        {
            if (sPokeMoverContext->spriteMgr.incomingSprites[j] == NULL) {
                sPokeMoverContext->spriteMgr.incomingSprites[j] = AllocZeroed(sizeof(struct IncomingSprite));
                sPokeMoverContext->spriteMgr.incomingSprites[j]->paletteTag = PALTAG_BOXTITLE;
                sPokeMoverContext->spriteMgr.incomingSprites[j]->posX = 192 * direction + x + i * 32;
                sPokeMoverContext->spriteMgr.incomingSprites[j]->posY = 12;
                sPokeMoverContext->spriteMgr.incomingSprites[j]->priority = 2;
                sPokeMoverContext->spriteMgr.incomingSprites[j]->template = &sSpriteTemplate_BoxTitle;
                sPokeMoverContext->spriteMgr.incomingSprites[j]->subpriority = 24;
                sPokeMoverContext->spriteMgr.incomingSprites[j]->tileNum = (tileOffset + i * 32 * 16 / 2) / TILE_SIZE_4BPP;
                break;
            }
        }
    }
    Free(boxTitleTile);
}

static void DrawBoxMonIcon(u8 box) {
    u16 iconSpecies;
    s32 tileOffset, i, j;
    u8 spriteId;
    bool8 isTransparent;
    struct SpriteTemplate template;
    const struct BoxDataSource *dataSource = sPokeMoverContext->boxView.boxDataSource;
    for (i = 0; i < dataSource->maxRow; i++) {
        for (j = 0; j < dataSource->maxColumn; j++) {
            isTransparent = FALSE;
            iconSpecies = dataSource->GetBoxMonIconSpecies(box, i, j, &isTransparent);
            if (iconSpecies == 0)
                continue;
            template = sSpriteTemplate_MonIcon;
            template.paletteTag += gRomHeader->monIconPaletteIds[iconSpecies];
            tileOffset = AllocSpriteTileOffset();
            if (tileOffset == -1)
                break;
            CpuCopy32(GetMonIconTiles(iconSpecies, TRUE), (void *)(OBJ_VRAM0) + tileOffset * TILE_SIZE_4BPP, 0x200);
            spriteId = CreateSprite(&template,
                                    100 + dataSource->leftBorder + dataSource->iconWidth * j,
                                    28 + dataSource->topBorder + dataSource->iconHeight * i, 19 - j);
            OnSpriteCreated(spriteId);
            gSprites[spriteId].oam.priority = 2;
            gSprites[spriteId].oam.tileNum = tileOffset;
            if (isTransparent)
                gSprites[spriteId].oam.objMode = ST_OAM_OBJ_BLEND;
        }
    }
}

static void DrawBoxTitle(u8 box) {
    u8 boxTitle[16], *boxTitleTile, x, spriteId;
    u32 tileOffset;
    int i;
    struct SpriteTemplate template;
    const struct BoxDataSource *dataSource = sPokeMoverContext->boxView.boxDataSource;
    dataSource->GetLocalBoxTitle(box, boxTitle);
    boxTitleTile = AllocZeroed(64 * 16 / 2);
    DrawTextWindowAndBufferTiles(boxTitle, boxTitleTile, 0, 0, 2);
    tileOffset = sPokeMoverContext->boxView.slot == 0 ? BOX_TITLE_SLOT_0_TILE_OFFSET : BOX_TITLE_SLOT_1_TILE_OFFSET;
    CpuCopy16(boxTitleTile, (void *)(OBJ_VRAM0 + tileOffset), 512);

    template = sSpriteTemplate_BoxTitle;
    template.paletteTag = PALTAG_BOXTITLE;
    template.anims = gDummySpriteAnimTable;
    x = DISPLAY_WIDTH - 64 - GetStringWidth(FONT_NORMAL, boxTitle, 0) / 2;
    for (i = 0; i < 2; i++) {
        spriteId = CreateSprite(&template, x + i * 32, 12, 24);
        gSprites[spriteId].oam.tileNum = (tileOffset + i * 32 * 16 / 2) / TILE_SIZE_4BPP;
        OnSpriteCreated(spriteId);
    }
    Free(boxTitleTile);
}

static bool8 DrawBoxWallpaper(int direction) {
    const struct Wallpaper *wallpaper;
    u32 tilemapLeft = ((sPokeMoverContext->boxView.bg2PosX + 80) / 8 + direction * 24) & 0x3F;
    bool8 isWaldaWallpaper;
    u16 tempPalette[32];
    wallpaper = sPokeMoverContext->boxView.boxDataSource->GetWallpaper(sPokeMoverContext->boxView.currentBox, &isWaldaWallpaper);
    switch (sPokeMoverContext->boxView.state)
    {
        case 0:
            DecompressAndLoadBgGfxDirectly(wallpaper->tiles, TILEMAP_WALLPAPER_BEGIN + sPokeMoverContext->boxView.slot * 128);
            sPokeMoverContext->boxView.state++;
            break;
        case 1:
            if (FreeTempTileDataBuffersIfPossible())
                return FALSE;
            sPokeMoverContext->boxView.state++;
            break;
        case 2:
            if (isWaldaWallpaper)
            {
                memcpy(tempPalette, wallpaper->palettes, sizeof(tempPalette));
                tempPalette[17] = tempPalette[1] = gSaveBlock1Ptr->waldaPhrase.colors[0];
                tempPalette[18] = tempPalette[2] = gSaveBlock1Ptr->waldaPhrase.colors[1];
                LoadPalette(tempPalette, BG_PLTT_ID(4) + BG_PLTT_ID(sPokeMoverContext->boxView.slot * 2), 2 * PLTT_SIZE_4BPP);
                DecompressAndLoadBgGfxDirectly(gRomResource->waldaWallpaperIcons[GetWaldaWallpaperIconId()], TILEMAP_WALLPAPER_BEGIN + sPokeMoverContext->boxView.slot * 128 + 64);
            }
            else {
                bool8 isRubySapp = gRomHeader->version == VERSION_RUBY || gRomHeader->version == VERSION_SAPPHIRE;
                LoadPalette(wallpaper->palettes + 16 * isRubySapp, BG_PLTT_ID(4) + BG_PLTT_ID(sPokeMoverContext->boxView.slot * 2), 2 * PLTT_SIZE_4BPP);
            }
            sPokeMoverContext->boxView.state++;
            break;
        case 3:
            LZ77UnCompWram(wallpaper->tilemap, sPokeMoverContext->boxView.wallpaperTilemap);
            CopyRectToBgTilemapBufferRect(2, sPokeMoverContext->boxView.wallpaperTilemap, 0, 0, 20, 18, tilemapLeft, 0, 20, 18, 17, TILEMAP_WALLPAPER_BEGIN + sPokeMoverContext->boxView.slot * 128, 3 + sPokeMoverContext->boxView.slot * 2);
            if (direction == 1) {
                FillBgTilemapBufferRect(2, 0, tilemapLeft + 20, 0, 4, 0x12, 17);
            }
            else if (direction == -1) {
                FillBgTilemapBufferRect(2, 0, tilemapLeft - 4, 0, 4, 0x12, 17);
            }
            CopyBgTilemapBufferToVram(2);
            sPokeMoverContext->boxView.state = 0;
            return TRUE;
    }
    return FALSE;
}

static void DrawLegalityViewMonIcon(s32 relativeIndex, struct LegalityCheckResult * legalityCheckresult) {
    struct BoxPokemon *mon;
    u32 personality;
    u16 species, iconSpecies;
    struct SpriteTemplate template;
    u8 spriteId;
    s32 k, tileOffset;
    mon = &gPokemonStoragePtr->boxes[sPokeMoverContext->boxView.localBox][legalityCheckresult->boxMonIndex];
    personality = GetBoxMonData(mon, MON_DATA_PERSONALITY, NULL);
    species = GetBoxMonData(mon, MON_DATA_SPECIES_OR_EGG, NULL);
    iconSpecies = GetIconSpecies(species, personality);
    if (relativeIndex >= 0 && relativeIndex < 3) {
        template = sSpriteTemplate_MonIcon;
        template.paletteTag += gRomHeader->monIconPaletteIds[iconSpecies];
        tileOffset = AllocSpriteTileOffset();
        if (tileOffset == -1)
            return;
        CpuCopy32(GetMonIconTiles(iconSpecies, TRUE), (void *)(OBJ_VRAM0) + tileOffset * TILE_SIZE_4BPP, 0x200);
        spriteId = CreateSprite(&template, 24, relativeIndex * 48 + 24, 20);
        OnSpriteCreated(spriteId);
        gSprites[spriteId].oam.priority = 2;
        gSprites[spriteId].oam.tileNum = tileOffset;
    } 
    else {
        for (k = 0; k < ARRAY_COUNT(sPokeMoverContext->spriteMgr.incomingSprites);k++) {
            if (sPokeMoverContext->spriteMgr.incomingSprites[k] == NULL) {
                tileOffset = AllocSpriteTileOffset();
                if (tileOffset == -1)
                    return;
                CpuCopy32(GetMonIconTiles(iconSpecies, TRUE), (void *)(OBJ_VRAM0) + tileOffset * TILE_SIZE_4BPP, 0x200);
                sPokeMoverContext->spriteMgr.incomingSprites[k] = (struct IncomingSprite *)AllocZeroed(sizeof(struct IncomingSprite));
                sPokeMoverContext->spriteMgr.incomingSprites[k]->priority = 2;
                sPokeMoverContext->spriteMgr.incomingSprites[k]->subpriority = 20;
                sPokeMoverContext->spriteMgr.incomingSprites[k]->paletteTag = sSpriteTemplate_MonIcon.paletteTag + gRomHeader->monIconPaletteIds[iconSpecies];
                sPokeMoverContext->spriteMgr.incomingSprites[k]->posX = 24;
                sPokeMoverContext->spriteMgr.incomingSprites[k]->posY = relativeIndex * 48 + 20;
                sPokeMoverContext->spriteMgr.incomingSprites[k]->template = &sSpriteTemplate_MonIcon;
                sPokeMoverContext->spriteMgr.incomingSprites[k]->tileNum = tileOffset;
                sPokeMoverContext->spriteMgr.incomingSprites[k]->isTransparent = FALSE;
                break;
            }
        }
    }
}

static bool8 DrawLegalityView(u8 index, u8 tilemapTop, struct LegalityCheckResult * legalityCheckresult) {
    u8 str[16], level, move, language;
    s32 i;
    struct BoxPokemon *boxMon = &gPokemonStoragePtr->boxes[sPokeMoverContext->boxView.localBox][legalityCheckresult->boxMonIndex];
    static const u8 colors[3] = {TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_LIGHT_GRAY};
    index = index & 3;
    switch (sPokeMoverContext->legalityView.state)
    {
        case 0:
            FillWindowPixelBuffer(sPokeMoverContext->legalityView.windowIds[index], PIXEL_FILL(1));
            gWindows[sPokeMoverContext->legalityView.windowIds[index]].window.tilemapTop = tilemapTop;
            DrawStdFrameWithCustomTileAndPalette(sPokeMoverContext->legalityView.windowIds[index], FALSE, 11, 14);
            sPokeMoverContext->legalityView.state++;
            break;
        case 1:
            level = GetLevelFromBoxMonExp(boxMon);
            str[0] = 0x34;
            if (level < legalityCheckresult->minimumLevel) {
                str[1] = EXT_CTRL_CODE_BEGIN;
                str[2] = EXT_CTRL_CODE_COLOR;
                str[3] = TEXT_COLOR_RED;
                ConvertIntToDecimalStringN(&str[4], legalityCheckresult->minimumLevel, STR_CONV_MODE_LEFT_ALIGN, 3);
            }
            else {
                ConvertIntToDecimalStringN(&str[1], level, STR_CONV_MODE_LEFT_ALIGN, 3);            }
            DrawText(sPokeMoverContext->legalityView.windowIds[index], str, 32, 0, NULL, TRUE);
            sPokeMoverContext->legalityView.state++;
            break;
        case 2:
            GetBoxMonData(boxMon, MON_DATA_NICKNAME, str);
            language = GetBoxMonData(boxMon, MON_DATA_LANGUAGE, NULL);
            if (language == LANGUAGE_ENGLISH)
                language = LANGUAGE_CHINESE;
            if (legalityCheckresult->nameInvalidFlags == 0) {
                DrawText(sPokeMoverContext->legalityView.windowIds[index], str, 32, 16, NULL, TRUE);
            }
            else {
                HighlightInvalidNameChars(str, gStringVar3, legalityCheckresult->nameInvalidFlags, language);
                DrawText(sPokeMoverContext->legalityView.windowIds[index], gStringVar3, 32, 16, NULL, TRUE);
            }
            sPokeMoverContext->legalityView.state++;
            break;
        case 3:
            for (i = 0; i < MAX_MON_MOVES; i++) {
                move = GetBoxMonData(boxMon, MON_DATA_MOVE1 + i, NULL);
                if (move == MOVE_NONE) continue;
                if ((legalityCheckresult->moveLegalFlags & (1 << i)) != 0)
                    DrawText(sPokeMoverContext->legalityView.windowIds[index], gMoveNames[move], 96 + (i & 1) * 64, (i / 2) * 16, NULL, TRUE);
                else
                    DrawText(sPokeMoverContext->legalityView.windowIds[index], gMoveNames[move], 96 + (i & 1) * 64, (i / 2) * 16, colors, TRUE);
            }
            sPokeMoverContext->legalityView.state++;
            break;
        case 4:
            sPokeMoverContext->legalityView.state = 0;
            return TRUE;
    }
    return FALSE;
}

static bool8 DrawLeftViewOfBox(struct Task *task) {
    static const u8 colors[3] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};
    static const void * const boxViewPokeTitleImages[][3] = {
        {boxViewPokeGoldTitle_Gfx, boxViewPokeGoldTitle_Tilemap, boxViewPokeGoldTitle_Pal},
        {boxViewPokeSliverTitle_Gfx, boxViewPokeSliverTitle_Tilemap, boxViewPokeSliverTitle_Pal},
        {boxViewPokeCrystalTitle_Gfx, boxViewPokeCrystalTitle_Tilemap, boxViewPokeCrystalTitle_Pal}
    };
    switch (task->tBoxState)
    {
    case 0:
        if (sPokeMoverContext->baseData.gameVer == VERSION_GOLD || sPokeMoverContext->baseData.gameVer == VERSION_SILVER)
        {
            DecompressAndLoadBgGfxDirectly(boxViewGBSkin_Gfx, TILEMAP_BOXVIEW_LEFT_VIEW_BEGIN);
            CopyRectToBgTilemapBufferRect(1, boxViewGBSkin_Tilemap, 0, 0, 10, 14, 0, 4, 10, 14, 17, TILEMAP_BOXVIEW_LEFT_VIEW_BEGIN, 9);
            LoadPalette(boxViewGBSkin_Pal, BG_PLTT_ID(9), 32 * 4);
        }
        else
        {
            DecompressAndLoadBgGfxDirectly(boxViewGBCSkin_Gfx, TILEMAP_BOXVIEW_LEFT_VIEW_BEGIN);
            CopyRectToBgTilemapBufferRect(1, boxViewGBCSkin_Tilemap, 0, 0, 10, 14, 0, 4, 10, 14, 17, TILEMAP_BOXVIEW_LEFT_VIEW_BEGIN, 9);
            LoadPalette(boxViewGBCSkin_Pal, BG_PLTT_ID(9), 32 * 4);
        }
        task->tBoxState++;
        break;
    case 1:
        if (!FreeTempTileDataBuffersIfPossible())
            task->tBoxState++;
        break;
    case 2:
        DecompressAndLoadBgGfxDirectly(boxViewPokeTitleImages[sPokeMoverContext->baseData.gameVer - 1][0], TILEMAP_BOXVIEW_GAME_TILE_BEGIN);
        CopyRectToBgTilemapBufferRect(1, boxViewPokeTitleImages[sPokeMoverContext->baseData.gameVer - 1][1], 0, 0, 10, 4, 0, 0, 10, 4, 17, TILEMAP_BOXVIEW_GAME_TILE_BEGIN, 1);
        LoadPalette(boxViewPokeTitleImages[sPokeMoverContext->baseData.gameVer - 1][2], BG_PLTT_ID(1), 32 * 2);
        task->tBoxState++;
        break;
    case 3:
        if (!FreeTempTileDataBuffersIfPossible())
            task->tBoxState++;
        break;
    case 4:
        ScheduleBgCopyTilemapToVram(1);
        task->tBoxState++;
        break;
    case 5:
        ShowBg(1);
        task->tBoxState++;
        break;
    case 6:
        ConvertIntToDecimalStringN(StringCopy(gStringVar3, gText_TrainerCardIDNo), (sPokeMoverContext->baseData.trainerID[0] << 8) | sPokeMoverContext->baseData.trainerID[1], STR_CONV_MODE_LEADING_ZEROS, 5);
        PutWindowTilemap(sPokeMoverContext->boxView.gscPlayerTrainerIdWinId);
        FillWindowPixelBuffer(sPokeMoverContext->boxView.gscPlayerTrainerIdWinId, PIXEL_FILL(0));
        DrawText(sPokeMoverContext->boxView.gscPlayerTrainerIdWinId, gStringVar3, 2, 3, colors, TRUE);
        task->tBoxState++;
        break;
    case 7:
        if (sPokeMoverContext->baseData.gameLang == LANGUAGE_JAPANESE)
        {
            gStringVar3[0] = EXT_CTRL_CODE_BEGIN;
            gStringVar3[1] = EXT_CTRL_CODE_JPN;
            DecodeGSCString(&gStringVar3[2], NAME_LENGTH_GSC_JP + 1, sPokeMoverContext->baseData.playerName, NAME_LENGTH_GSC_JP + 1, sPokeMoverContext->baseData.gameVer, sPokeMoverContext->baseData.gameLang);
        }
        else
            DecodeGSCString(gStringVar3, NAME_LENGTH_GSC_INTL + 1, sPokeMoverContext->baseData.playerName, NAME_LENGTH_GSC_INTL + 1, sPokeMoverContext->baseData.gameVer, sPokeMoverContext->baseData.gameLang);
        PutWindowTilemap(sPokeMoverContext->boxView.gscPlayerNameWinId);
        FillWindowPixelBuffer(sPokeMoverContext->boxView.gscPlayerNameWinId, PIXEL_FILL(0));
        DrawText(sPokeMoverContext->boxView.gscPlayerNameWinId, gStringVar3, 2, 3, colors, TRUE);
        task->tBoxState++;
        break;
    case 8:
        CpuCopy16(gscHeroIconSprite_Gfx, (void *)(OBJ_VRAM0 + GSC_HERO_ICON_TILE_OFFSET), 16 * 32 / 2);
        sPokeMoverContext->boxView.gscHeroIconSpriteId = CreateSprite(&sSpriteTemplate_GSCHeroIcon, 20, 60, 25);
        gSprites[sPokeMoverContext->boxView.gscHeroIconSpriteId].oam.tileNum = GSC_HERO_ICON_TILE_OFFSET / TILE_SIZE_4BPP;
        if (sPokeMoverContext->baseData.playerGender != 0)
            gSprites[sPokeMoverContext->boxView.gscHeroIconSpriteId].oam.tileNum += 4;
        task->tBoxState++;
        break;
    case 9:
        ScheduleBgCopyTilemapToVram(0);
        return TRUE;
    }
    return FALSE;
}

static void CB_NotifyStatusChanged(u8 changedStatus, bool8 dataHasReached) {
    sPokeMoverContext->linkStatus.status = changedStatus;
    sPokeMoverContext->linkStatus.newDataReached = sPokeMoverContext->linkStatus.newDataReached || dataHasReached;
}

static void CB_NotifyStatusChangedForGSCLink(u8 changedStatus, bool8 dataHasReached) {
    if (sPokeMoverContext->linkStatus.status != changedStatus)
        sPokeMoverContext->linkStatus.timer = 0;
    sPokeMoverContext->linkStatus.status = changedStatus;
    sPokeMoverContext->linkStatus.newDataReached = sPokeMoverContext->linkStatus.newDataReached || dataHasReached;
}

static bool8 DoScrollBox() {
    s8 spriteId;
    int i;
    SetGpuReg(REG_OFFSET_BG2HOFS, sPokeMoverContext->boxView.bg2PosX);
    sPokeMoverContext->boxView.bg2PosX += sPokeMoverContext->boxView.scrollSpeed;
    sPokeMoverContext->boxView.bg2PosX = sPokeMoverContext->boxView.bg2PosX & 0x1FF;
    for (i = 0; i < ARRAY_COUNT(sPokeMoverContext->spriteMgr.spriteIds); i++)
    {
        spriteId = sPokeMoverContext->spriteMgr.spriteIds[i];
        if (spriteId >= 0) {
            gSprites[spriteId].x += -sPokeMoverContext->boxView.scrollSpeed;
            if (gSprites[spriteId].x <= 64 || gSprites[spriteId].x >= 256) {
                OnSpriteRemoved(spriteId);
                FreeSpriteTilemem(gSprites[spriteId].oam.tileNum);
                DestroySprite(&gSprites[spriteId]);
            }
        }
    }
    for (i = 0; i < ARRAY_COUNT(sPokeMoverContext->spriteMgr.incomingSprites); i++)
    {
        if (sPokeMoverContext->spriteMgr.incomingSprites[i]) {
            sPokeMoverContext->spriteMgr.incomingSprites[i]->posX += -sPokeMoverContext->boxView.scrollSpeed;
            if (sPokeMoverContext->spriteMgr.incomingSprites[i]->posX > 64 && sPokeMoverContext->spriteMgr.incomingSprites[i]->posX < 256) {
                CreateSpriteFromIncoming(sPokeMoverContext->spriteMgr.incomingSprites[i]);
                FREE_AND_SET_NULL(sPokeMoverContext->spriteMgr.incomingSprites[i]);
            }
        }
    }
    return --sPokeMoverContext->boxView.scrollTimer == 0;
}

static bool8 DoScrollLegalityView()
{
    s32 i, spriteId;
    SetGpuReg(REG_OFFSET_BG2VOFS, sPokeMoverContext->legalityView.bg2PosY);
    sPokeMoverContext->legalityView.bg2PosY += sPokeMoverContext->legalityView.scrollSpeed;
    for (i = 0; i < ARRAY_COUNT(sPokeMoverContext->spriteMgr.spriteIds); i++)
    {
        spriteId = sPokeMoverContext->spriteMgr.spriteIds[i];
        if (spriteId >= 0) {
            gSprites[spriteId].y += -sPokeMoverContext->legalityView.scrollSpeed;
            if (gSprites[spriteId].y <= 0 || gSprites[spriteId].y >= 160) {
                OnSpriteRemoved(spriteId);
                FreeSpriteTilemem(gSprites[spriteId].oam.tileNum);
                DestroySprite(&gSprites[spriteId]);
            }
        }
    }
    for (i = 0; i < ARRAY_COUNT(sPokeMoverContext->spriteMgr.incomingSprites); i++)
    {
        if (sPokeMoverContext->spriteMgr.incomingSprites[i]) {
            sPokeMoverContext->spriteMgr.incomingSprites[i]->posY += -sPokeMoverContext->legalityView.scrollSpeed;
            if (sPokeMoverContext->spriteMgr.incomingSprites[i]->posY > 0 && sPokeMoverContext->spriteMgr.incomingSprites[i]->posY < 160) {
                CreateSpriteFromIncoming(sPokeMoverContext->spriteMgr.incomingSprites[i]);
                FREE_AND_SET_NULL(sPokeMoverContext->spriteMgr.incomingSprites[i]);
            }
        }
    }
    return --sPokeMoverContext->legalityView.scrollTimer == 0;
}

static bool8 HandleSendingTransferToolToGBA(struct Task * task) {
    bool8 result = FALSE;
    u32 size;
    u8 * ptr;
    switch (task->tSubState) {
        case 0:
            ptr = (u8*)malloc_and_decompress(gMultiBootProgram_GSC_Transfer_Tool, &size);
            SetWordTaskArg(0, 4, (u32)ptr);
            SetWordTaskArg(0, 6, size);
            task->tSubState++;
            break;
        case 1:
            ptr = (u8*)GetWordTaskArg(0, 4);
            size = GetWordTaskArg(0, 6);
            if (multiboot_normal(ptr, ptr + size) == MB_SUCCESS)
                task->tSubState++;
            else
                task->tSubState += 2;
            break;
        case 2:
            DrawDelayedMessage(0, gTextSendSuccessfully, 180);
            task->tSubState = 4;
            break;
        case 3:
            DrawDelayedMessage(0, gTextSendUnsuccessfully, 180);
            task->tSubState = 4;
            break;
        case 4:
            ptr = (u8*)GetWordTaskArg(0, 4);
            FREE_AND_SET_NULL(ptr);
            if (HandleDelayedMessage(A_BUTTON | B_BUTTON))
                result = TRUE;
            break;
    }
    return result;
}

static bool8 HandleSendingTransferToolToGBC(struct Task * task) {
    bool8 result = FALSE;
    s8 selectedOption;
    switch (task->tSubState) {
    case 0:
        ExitLinkPokeMover();
        FillWindowPixelBuffer(0, PIXEL_FILL(1));
        DrawMessage(gMsgUsageStep1OfSendingTransferToolOverExploit, 2);
        task->tSubState++;
        break;
    case 1:
        if (HandleMessage())
        {
            DrawOptions(sPokeMoverLanguageOptions, ARRAY_COUNT(sPokeMoverLanguageOptions), &sWindowTemplate_PokeMoverLanguageOptions, &task->tWindowId);
            CopyWindowToVram(task->tWindowId, COPYWIN_FULL);
            task->tSubState++;
        }
        break;
    case 2:
        selectedOption = Menu_ProcessInput();
        if (selectedOption == 0 || selectedOption == 1) {
            SetupLinkGSCTrade(selectedOption == 0, CB_NotifyStatusChangedForGSCLink);
            ClearStdWindowAndFrame(task->tWindowId, TRUE);
            RemoveWindow(task->tWindowId);
            task->tSubState++;
        }
        break;
    case 3:
        FillWindowPixelBuffer(0, PIXEL_FILL(1));
        DrawMessage(gMsgUsageStep2OfSendingTransferToolOverExploit, 2);
        task->tSubState++;
        break;
    case 4:
        if (HandleMessage())
        {
            TryHandshakeWithGSC();
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            DrawText(0, gTextLinkToGSC, 0, 0, NULL, TRUE);
            sPokeMoverContext->linkStatus.status = ~STATE_IDLE;
            sPokeMoverContext->linkStatus.timer = 0;
            task->tSubState++;
        }
        break;
    case 5:
        if (sPokeMoverContext->linkStatus.status == STATE_IDLE) {
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            DrawMessage(gMsgUsageStep3OfSendingTransferToolOverExploit, 2);
            DecompressAndLoadBgGfxDirectly(crystalExploitTotorial_Gfx, TILEMAP_COMMON_BEGIN);
            LoadPalette(crystalExploitTotorial_Pal, BG_PLTT_ID(8), 32);
            task->tSubState++;
        }
        else if (sPokeMoverContext->linkStatus.timer++ >= 1200) {
            DrawDelayedMessage(0, gTextSendToGBCUnsuccessfully0, 180);
            ExitLinkGSCTrade();
            task->tSubState = 9;
        }
        break;
    case 6:
        if (!FreeTempTileDataBuffersIfPossible() && HandleMessage()) {
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            DrawMessage(gMsgUsageStep4OfSendingTransferToolOverExploit, 2);
            task->tTutorialFrame = 0;
            task->tTutorialCounter = 0;
            task->tSubState++;
        }
        break;
    case 7:
        if (HandleMessage()) {
            TryEnteringGSCTradeView();
            FillBgTilemapBufferRect(0, 0, 5, 2, 20, 10, 0);
            ScheduleBgCopyTilemapToVram(0);
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            DrawText(0, gTextSending, 0, 0, NULL, TRUE);
            sPokeMoverContext->linkStatus.timer = 0;
            task->tSubState++;
        }
        else {
            if (task->tTutorialCounter == 0) {
                task->tTutorialCounter = 90;
                CopyRectToBgTilemapBufferRect(0, (const u16 *)&crystalExploitTotorial_Tilemap[0] + 20 * 10 * task->tTutorialFrame, 0, 0, 20, 10, 5, 2, 20, 10, 0, TILEMAP_COMMON_BEGIN - 1, 8);
                ScheduleBgCopyTilemapToVram(0);
                if (++task->tTutorialFrame >= 5)
                    task->tTutorialFrame = 0;
            }
            task->tTutorialCounter--;
        }
        break;
    case 8:
        if (sPokeMoverContext->linkStatus.status == STATE_SENDING_PAYLOAD_COMPLETELY) {
            DrawDelayedMessage(0, gTextSendSuccessfully, 180);
            ExitLinkGSCTrade();
            task->tSubState++;
        }
        else if (sPokeMoverContext->linkStatus.timer++ >= 600) {
            DrawDelayedMessage(0, gTextSendToGBCUnsuccessfully1, 180);
            ExitLinkGSCTrade();
            task->tSubState++;
        }
        if (sPokeMoverContext->linkStatus.newDataReached)
            sPokeMoverContext->linkStatus.timer = 0;
        sPokeMoverContext->linkStatus.newDataReached = FALSE;
        break;
    case 9:
        if (HandleDelayedMessage(A_BUTTON))
            result = TRUE;
        break;
    }
    return result;
}

static s32 HandleScrollableBoxView(struct Task * task){
    s8 nextBox;
    u8 maxBox;
    switch (task->tBoxState)
    {
        case 0:
            sPokeMoverContext->boxView.state = 0;
            task->tBoxState++;
            InitSpriteMgr();
            DrawBoxMonIcon(sPokeMoverContext->boxView.currentBox);
            break;
        case 1:
            DrawBoxTitle(sPokeMoverContext->boxView.currentBox);
            task->tBoxState++;
            break;
        case 2:
            if (DrawBoxWallpaper(0))
            {
                sPokeMoverContext->boxView.slot = !sPokeMoverContext->boxView.slot;
                task->tBoxState++;
            }
            break;
        case 3:
            if (JOY_NEW(DPAD_LEFT | DPAD_RIGHT)) {
                maxBox = sPokeMoverContext->boxView.boxDataSource->maxBoxCount;
                if (JOY_NEW(DPAD_LEFT))
                {
                    sPokeMoverContext->boxView.direction = -1;
                    nextBox = sPokeMoverContext->boxView.currentBox - 1;
                    if (nextBox < 0)
                        nextBox = maxBox - 1;
                }
                else {
                    sPokeMoverContext->boxView.direction = 1;
                    nextBox = sPokeMoverContext->boxView.currentBox + 1;
                    if (nextBox >= maxBox)
                        nextBox = 0;
                }
                if (nextBox != sPokeMoverContext->boxView.latestBox) 
                    task->tBoxState = 4;
                else 
                    task->tBoxState = 5;
                sPokeMoverContext->boxView.latestBox = sPokeMoverContext->boxView.currentBox;
                sPokeMoverContext->boxView.currentBox = nextBox;
            }
            else if (JOY_NEW(A_BUTTON)) 
                return 1;
            else if (JOY_NEW(B_BUTTON))
                return -1;
            break;
        case 4:
            if (DrawBoxWallpaper(sPokeMoverContext->boxView.direction)) {
                task->tBoxState++;
            }
            break;
        case 5:
            CreateIncomingBoxTitle(sPokeMoverContext->boxView.direction);
            task->tBoxState++;
            break;
        case 6:
            CreateIncomingBoxMonIcon(sPokeMoverContext->boxView.currentBox, sPokeMoverContext->boxView.direction);
            sPokeMoverContext->boxView.slot = !sPokeMoverContext->boxView.slot;
            sPokeMoverContext->boxView.scrollSpeed = sPokeMoverContext->boxView.direction * 6;
            sPokeMoverContext->boxView.scrollTimer = 192 / 6;
            task->tBoxState++;
            break;
        case 7:
            if (DoScrollBox()) {
                task->tBoxState = 8;
            }
            break;
        case 8:
            SetGpuReg(REG_OFFSET_BG2HOFS, sPokeMoverContext->boxView.bg2PosX);
            task->tBoxState = 3;
            break;
    }
    return 0;
}


static int HandleLegalityView(struct Task * task) {
    s32 i = 0;
    u8 tilemapTop;
    u16 tilemap[BOTTOM_HINT_WIDTH * BOTTOM_HINT_HEIGHT];
    switch (task->tLegalityViewState)
    {
        case 0:
            memset(&sPokeMoverContext->legalityView, 0, sizeof(sPokeMoverContext->legalityView));
            for (i = 0; i < ARRAY_COUNT(sPokeMoverContext->legalityView.windowIds); i++) {
                sPokeMoverContext->legalityView.windowIds[i] = AddWindow(&sWindowTemplates_LegalityView[i]);
            }
            for (i = 0; i < ARRAY_COUNT(tilemap); i++)
                tilemap[i] = (u16)i;
            DecompressAndLoadBgGfxDirectly(boxViewBottomBar2_Gfx, sWindowTemplate_LegalityViewHint.baseBlock);
            CopyRectToBgTilemapBufferRect(0, tilemap, 0, 0, BOTTOM_HINT_WIDTH, BOTTOM_HINT_HEIGHT, sWindowTemplate_LegalityViewHint.tilemapLeft, sWindowTemplate_LegalityViewHint.tilemapTop, BOTTOM_HINT_WIDTH, BOTTOM_HINT_HEIGHT, 17, TILEMAP_COMMON_BEGIN, sWindowTemplate_LegalityViewHint.paletteNum);
            LoadPalette(boxViewBottomBar2_Pal, PLTT_ID(sWindowTemplate_LegalityViewHint.paletteNum), 32);
            SetGpuReg(REG_OFFSET_BG2VOFS, 0);
            SetGpuReg(REG_OFFSET_BG2HOFS, 0);
            sPokeMoverContext->boxView.bg2PosX = 0;
            task->tLegalityViewState++;
            break;
        case 1:
            if (!FreeTempTileDataBuffersIfPossible())
                task->tLegalityViewState++;
            break;
        case 2:
        case 3:
        case 4:
            i = task->tLegalityViewState - 2;
            if (i >= sPokeMoverContext->checkResultNum)
                task->tLegalityViewState = 5;
            else if (DrawLegalityView(i, 1 + i * 6, &sPokeMoverContext->checkResults[i])) {
                task->tLegalityViewState++;
            }
            break;
        case 5:
        case 6:
        case 7:
            i = task->tLegalityViewState - 5;
            if (i >= sPokeMoverContext->checkResultNum)
                task->tLegalityViewState = 8;
            else {
                DrawLegalityViewMonIcon(i, &sPokeMoverContext->checkResults[i]);
                task->tLegalityViewState++;
            }
            break;
        case 8:
            ScheduleBgCopyTilemapToVram(0);
            ScheduleBgCopyTilemapToVram(2);
            task->tLegalityViewState++;
            break;
        case 9:
            if (JOY_NEW(DPAD_UP | DPAD_DOWN)) {
                if (JOY_NEW(DPAD_UP) && sPokeMoverContext->legalityView.currentIndex > 0) {
                    sPokeMoverContext->legalityView.isScrollUp = TRUE;
                    task->tLegalityViewState = --sPokeMoverContext->legalityView.currentIndex != sPokeMoverContext->legalityView.latestIndex ? 10 : 11;
                }
                else if (JOY_NEW(DPAD_DOWN) && sPokeMoverContext->legalityView.currentIndex + 3 < sPokeMoverContext->checkResultNum) {
                    sPokeMoverContext->legalityView.isScrollUp = FALSE;
                    task->tLegalityViewState = ++sPokeMoverContext->legalityView.currentIndex != sPokeMoverContext->legalityView.latestIndex ? 10 : 11;
                }
            }
            else if (JOY_NEW(A_BUTTON | B_BUTTON)) {
                for (i = 0; i < sizeof(sPokeMoverContext->legalityView.windowIds); i++) {
                    RemoveWindow(sPokeMoverContext->legalityView.windowIds[i]);
                }
                FillBgTilemapBufferRect(
                    sWindowTemplate_LegalityViewHint.bg,
                    0,
                    sWindowTemplate_LegalityViewHint.tilemapLeft,
                    sWindowTemplate_LegalityViewHint.tilemapTop,
                    sWindowTemplate_LegalityViewHint.width,
                    sWindowTemplate_LegalityViewHint.height,
                    sWindowTemplate_LegalityViewHint.paletteNum);
                CpuFastFill(0, sPokeMoverContext->boxView.bg2Tilemap, sizeof(sPokeMoverContext->boxView.bg2Tilemap));
                task->tLegalityViewState = JOY_NEW(A_BUTTON) ? 14 : 15;
            }
            break;
        case 10:
            tilemapTop = (sPokeMoverContext->legalityView.bg2PosY / 8) + (sPokeMoverContext->legalityView.isScrollUp ? -5 : +19);
            tilemapTop &= 0x1F;
            i = sPokeMoverContext->legalityView.latestIndex + (sPokeMoverContext->legalityView.isScrollUp ? -1 : +3);
            if (DrawLegalityView(i, tilemapTop, &sPokeMoverContext->checkResults[i])) {
                ScheduleBgCopyTilemapToVram(2);
                task->tLegalityViewState++;
            }
            break;
        case 11:
            sPokeMoverContext->legalityView.scrollSpeed = sPokeMoverContext->legalityView.isScrollUp ? -6 : 6;
            sPokeMoverContext->legalityView.scrollTimer = 8;
            i = sPokeMoverContext->legalityView.latestIndex + (sPokeMoverContext->legalityView.isScrollUp ? -1 : +3);
            DrawLegalityViewMonIcon(sPokeMoverContext->legalityView.isScrollUp ? -1 : 3, &sPokeMoverContext->checkResults[i]);
            task->tLegalityViewState++;
            break;
        case 12:
            if (DoScrollLegalityView()) {
                sPokeMoverContext->legalityView.latestIndex = sPokeMoverContext->legalityView.currentIndex;
                task->tLegalityViewState++;
            }
            break;
        case 13:
            SetGpuReg(REG_OFFSET_BG2VOFS, sPokeMoverContext->legalityView.bg2PosY);
            task->tLegalityViewState = 9;
            break;
        case 14:
        case 15:
            InitSpriteMgr();
            ScheduleBgCopyTilemapToVram(0);
            ScheduleBgCopyTilemapToVram(2);
            task->tLegalityViewState += 2;
            break;
        case 16:
        case 17:
            SetGpuReg(REG_OFFSET_BG2VOFS, 0);
            return task->tLegalityViewState == 16 ? 1 : -1;
    }
    return 0;
}


static int HandleRecvingData(bool8 isBoxData) {
    u8 status = sPokeMoverContext->linkStatus.status;
    bool8 newDataReached = sPokeMoverContext->linkStatus.newDataReached;
    u8 *dest = NULL;
    size_t size = 0;
    switch (sPokeMoverContext->linkStatus.state)
    {
        case 0:
            if (isBoxData)
                ExecBufferCommand((sPokeMoverContext->linkStatus.currentOffset << 4) | sPokeMoverContext->linkStatus.currentBox);
            else
                ExecBufferCommand((sPokeMoverContext->linkStatus.currentOffset << 4) | 0xF);
            sPokeMoverContext->linkStatus.state++;
            sPokeMoverContext->linkStatus.timer = 0;
            break;
        case 1:
            if (status == TRANSFER_IDLE && sPokeMoverContext->linkStatus.timer++ > 180)
                return -1;
            else if (status != TRANSFER_IDLE)
            {
                sPokeMoverContext->linkStatus.state++;
                sPokeMoverContext->linkStatus.timer = 0;
                sPokeMoverContext->linkStatus.newDataReached = FALSE;
            }
            break;
        case 2:
            if (status == TRANSFER_BUSY 
                && !newDataReached 
                && sPokeMoverContext->linkStatus.timer++ > 180)
                return -1;
            else if (status == TRANSFER_COMPLETE)
                sPokeMoverContext->linkStatus.state++;
            else if (status == TRANSFER_FAILED) {
                if (++sPokeMoverContext->linkStatus.retry < 4)
                    sPokeMoverContext->linkStatus.state = 0;
                else
                    return -1;
            }
            sPokeMoverContext->linkStatus.newDataReached = FALSE;
            break;
        case 3:
            if (isBoxData) {
                dest = (u8 *)&sPokeMoverContext->boxData + sPokeMoverContext->linkStatus.currentOffset * MAX_BUFFER_SIZE;
                size = min(GET_BOX_SIZE - sPokeMoverContext->linkStatus.currentOffset * MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);
            }
            else {
                dest = (u8 *)&sPokeMoverContext->baseData + sPokeMoverContext->linkStatus.currentOffset * MAX_BUFFER_SIZE;
                size = min(sizeof(struct GSCBaseData) - sPokeMoverContext->linkStatus.currentOffset * MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);
            }
            memcpy(dest, GetLinkPokeMoverBuffer(), size);
            if (size < MAX_BUFFER_SIZE)
                return 1;
            else {
                sPokeMoverContext->linkStatus.currentOffset++;
                sPokeMoverContext->linkStatus.state = 0;
            }
            break;
    }
    return 0;
}

static int HandleEraseBoxMon(u8 box, u8 *erasedIndices, u16 length)
{
    u8 status = sPokeMoverContext->linkStatus.status;
    bool8 newDataReached = sPokeMoverContext->linkStatus.newDataReached;
    u16 oldRecvSize;
    switch (sPokeMoverContext->linkStatus.state)
    {
        case 0:
            ExecEraseCommand(0xA0 | (box & 0xF), erasedIndices, length);
            sPokeMoverContext->linkStatus.state++;
            sPokeMoverContext->linkStatus.timer = 0;
            break;
        case 1:
            if (status == TRANSFER_IDLE && sPokeMoverContext->linkStatus.timer++ > 180)
                return -1;
            else if (status != TRANSFER_IDLE)
            {
                sPokeMoverContext->linkStatus.state++;
                sPokeMoverContext->linkStatus.timer = 0;
                sPokeMoverContext->linkStatus.newDataReached = FALSE;
            }
            break;
        case 2:
            if (status == TRANSFER_BUSY 
                && !newDataReached 
                && sPokeMoverContext->linkStatus.timer++ > 180)
                return -1;
            else if (status == TRANSFER_FAILED) {
                if (++sPokeMoverContext->linkStatus.retry < 4) {
                    sPokeMoverContext->linkStatus.state = 0;
                }
                else
                    return -1;
            }
            else if (status == TRANSFER_WAIT_SYNC) {
                sPokeMoverContext->linkStatus.state++;
                sPokeMoverContext->linkStatus.timer = 0;
                return 1;
            }
            sPokeMoverContext->linkStatus.newDataReached = FALSE;
            break;
        case 3:
            if (!newDataReached 
                && sPokeMoverContext->linkStatus.timer++ > 180)
                return -1;
            else if (status == TRANSFER_SYNC_COMPLETE)
                return 2;
            sPokeMoverContext->linkStatus.newDataReached = FALSE;
            break;
    }
    return 0;
}

static int HandleRecvingView(struct Task *task) {
    s32 result, i;
    u8 tempBuff[32];
    const void *ptr;
    struct BoxMonGSCBase *bases;
    u16 tilemap[BOTTOM_HINT_WIDTH * BOTTOM_HINT_HEIGHT];
    switch (task->tSubState)
    {
        case 0:
            memset(&sPokeMoverContext->linkStatus, 0, sizeof(sPokeMoverContext->linkStatus));
            task->tSubState++;
            break;
        case 1:
        case 24:
            result = HandleRecvingData(task->tSubState != 1);
            if (result == -1) {
                DrawDelayedMessage(0, gTextRecvUnsuccessfully, 180);
                task->tSubState++;
            }
            else if (result == 1)
            {
                DrawDelayedMessage(0, gTextRecvSuccessfully, 180);
                task->tSubState += 2;
            }
            break;
        case 2:
        case 25:
            if (HandleDelayedMessage(A_BUTTON | B_BUTTON))
                return -1;
            break;
        case 3:
        case 26:
            if (HandleDelayedMessage(A_BUTTON))
                task->tSubState++;
            break;
        case 4:
            ClearDialogWindowAndFrame(0, TRUE);
            task->tBoxState = 0;
            task->tSubState++;
            break;
        case 5:
            if (DrawLeftViewOfBox(task)) {
                task->tSubState++;
            }
            break;
        case 6:
            sPokeMoverContext->boxView.currentBox = sPokeMoverContext->boxView.gscBox;
            sPokeMoverContext->boxView.latestBox = 255;
            if (sPokeMoverContext->baseData.gameLang == LANGUAGE_JAPANESE)
                sPokeMoverContext->boxView.boxDataSource = &gscJPNBoxDataSource;
            else
                sPokeMoverContext->boxView.boxDataSource = &gscINTLBoxDataSource;
            task->tSubState++;
            task->tBoxState = 0;
            break;
        case 7:
        case 15:
            ptr = task->tSubState == 7 ? boxViewBottomBar0_Gfx : boxViewBottomBar1_Gfx;
            DecompressAndLoadBgGfxDirectly(ptr, sWindowTemplates[0].baseBlock);
            for (i = 0; i < ARRAY_COUNT(tilemap); i++)
                tilemap[i] = (u16)i;
            CopyRectToBgTilemapBufferRect(1, tilemap, 0, 0, BOTTOM_HINT_WIDTH, BOTTOM_HINT_HEIGHT, sWindowTemplates[0].tilemapLeft, sWindowTemplates[0].tilemapTop, BOTTOM_HINT_WIDTH, BOTTOM_HINT_HEIGHT, 17, sWindowTemplates[0].baseBlock, sWindowTemplates[0].paletteNum);
            ptr = task->tSubState == 7 ? boxViewBottomBar0_Pal : boxViewBottomBar1_Pal;
            LoadPalette(ptr, BG_PLTT_ID(sWindowTemplates[0].paletteNum), 32);
            task->tSubState++;
            break;
        case 8:
        case 16:
            if (!FreeTempTileDataBuffersIfPossible()) {
                CopyBgTilemapBufferToVram(1);
                task->tSubState++;
            }
            break;
        case 9:
        case 17:
            result = HandleScrollableBoxView(task);
            if (result == 1 || (result == -1 && task->tSubState == 9))
            {
                DrawStandardTextBox();
            }
            if (result == 1) {
                if (task->tSubState == 9)
                    task->tSubState += 3;
                else
                    task->tSubState++;
            }
            else if (result == -1) {
                if (task->tSubState == 9) {
                    sPokeMoverContext->boxView.gscBox = sPokeMoverContext->boxView.currentBox;
                    task->tSubState++;
                }
                else {
                    sPokeMoverContext->boxView.localBox = sPokeMoverContext->boxView.currentBox;
                    task->tSubState = 6;
                }
            }
            break;
        case 10:
        case 21:
            InitSpriteMgr();
            DestroySprite(&gSprites[sPokeMoverContext->boxView.gscHeroIconSpriteId]);
            CpuFastFill(0, sPokeMoverContext->boxView.bg2Tilemap, sizeof(sPokeMoverContext->boxView.bg2Tilemap));
            ScheduleBgCopyTilemapToVram(2);
            task->tSubState++;
            break;
        case 11:
        case 22:
            ClearWindowTilemap(sPokeMoverContext->boxView.gscPlayerNameWinId);
            CopyWindowToVram(sPokeMoverContext->boxView.gscPlayerNameWinId, COPYWIN_MAP);
            ClearWindowTilemap(sPokeMoverContext->boxView.gscPlayerTrainerIdWinId);
            CopyWindowToVram(sPokeMoverContext->boxView.gscPlayerTrainerIdWinId, COPYWIN_MAP);
            HideBg(1);
            if (task->tSubState == 11)
                return -1;
            task->tSubState++;
            break;
        case 12:
        case 18:
            ptr = task->tSubState == 12 ? gTextConfirmTransferingBox : gTextConfirmReceivingBox;
            sPokeMoverContext->boxView.boxDataSource->GetLocalBoxTitle(sPokeMoverContext->boxView.currentBox, tempBuff);
            StringCopy(gStringVar1, tempBuff);
            StringExpandPlaceholders(gStringVar2, ptr);
            DrawText(0, gStringVar2, 0, 0, NULL, TRUE);
            CreateYesNoMenu(&sWindowTemplates[3], 11, 14, 0);
            task->tSubState++;
            break;
        case 13:
            switch (Menu_ProcessInputNoWrapClearOnChoose()) {
                case MENU_B_PRESSED:
                case 1: // No
                    ClearDialogWindowAndFrame(0, TRUE);
                    task->tSubState = 9;
                    break;
                case 0: // Yes
                    if (CalcGSCTransferableBoxMonCount(sPokeMoverContext->boxView.currentBox) > 0) {
                        ClearDialogWindowAndFrame(0, TRUE);
                        sPokeMoverContext->boxView.gscBox = sPokeMoverContext->boxView.currentBox;
                        sPokeMoverContext->boxView.currentBox = sPokeMoverContext->boxView.localBox;
                        sPokeMoverContext->boxView.latestBox = 255;
                        sPokeMoverContext->boxView.boxDataSource = &localBoxDataSource;
                        task->tSubState += 2;
                        task->tBoxState = 0;
                    }
                    else {
                        DrawDelayedMessage(0, gTextNoTransferableMon, 180);
                        task->tSubState++;
                    }
                    break;
            }
            break;
        case 19:
            switch (Menu_ProcessInputNoWrapClearOnChoose()) {
                case MENU_B_PRESSED:
                case 1: // No
                    ClearDialogWindowAndFrame(0, TRUE);
                    task->tSubState = 17;
                    break;
                case 0: // Yes
                    if (CheckLocalBoxHasEnoughSpace()) {
                        sPokeMoverContext->boxView.localBox = sPokeMoverContext->boxView.currentBox;
                        task->tSubState += 2;
                    }
                    else {
                        DrawDelayedMessage(0, gTextNoEnoughSpace, 180);
                        task->tSubState++;
                    }
                    break;
            }
            break;
        case 14:
        case 20:
            if (HandleDelayedMessage(A_BUTTON | B_BUTTON)) {
                ClearDialogWindowAndFrame(0, TRUE);
                if (task->tSubState == 14)
                    task->tSubState = 9;
                else
                    task->tSubState = 17;
            }
            break;
        case 23:
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            DrawText(0, gTextLinking, 0, 0, NULL, TRUE);
            memset(&sPokeMoverContext->linkStatus, 0, sizeof(sPokeMoverContext->linkStatus));
            sPokeMoverContext->linkStatus.currentBox = sPokeMoverContext->boxView.gscBox;
            task->tSubState++;
            break;
        case 27:
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            DrawText(0, gTextProcessing, 0, 0, NULL, TRUE);
            task->tSubState++;
            break;
        case 28:
            if (sPokeMoverContext->baseData.gameLang == LANGUAGE_JAPANESE)
                bases = sPokeMoverContext->baseData.boxes.jp[sPokeMoverContext->boxView.gscBox];
            else
                bases = sPokeMoverContext->baseData.boxes.intl[sPokeMoverContext->boxView.gscBox];
            result = DoGSCBoxMonConversion(sPokeMoverContext->boxView.localBox, bases, tempBuff);
            if (result > 0) {
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                DrawMessage(gTextUsageOfLegalityView, 2);
                task->tTransferingMonNum = result;
                memcpy(sPokeMoverContext->convertedGSCBoxMons, tempBuff, task->tTransferingMonNum);
                task->tSubState += 2;
            }
            else {
                DrawDelayedMessage(0, gTextNoTransferableMon, 180);
                task->tSubState++;
            }
            break;
        case 29:
            if (HandleDelayedMessage(A_BUTTON | B_BUTTON)) {
                task->tSubState = 4;
            }
            break;
        case 30:
            if (HandleMessage())
            {
                ClearDialogWindowAndFrame(0, TRUE);
                task->tLegalityViewState = 0;
                task->tSubState++;
            }
            break;
        case 31:
            result = HandleLegalityView(task);
            if (result == 1) {
                for (i = 0; i < sPokeMoverContext->checkResultNum; i++) {
                    MakeGSCBoxMonLegal(&gPokemonStoragePtr->boxes[sPokeMoverContext->boxView.localBox][sPokeMoverContext->checkResults[i].boxMonIndex], &sPokeMoverContext->checkResults[i]);
                }
                UpdatePokedex();
                task->tSubState++;
            }
            else if (result == -1) {
                for (i = 0; i < sPokeMoverContext->checkResultNum; i++) {
                    ZeroBoxMonAt(sPokeMoverContext->boxView.localBox, sPokeMoverContext->checkResults[i].boxMonIndex);
                }
                task->tSubState = 4;
            }
            break;
        case 32:
            DrawStandardTextBox();
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            DrawText(0, gTextSaving, 0, 0, NULL, TRUE);
            task->tSubState++;
            break;
        case 33:
            memset(&sPokeMoverContext->linkStatus, 0, sizeof(sPokeMoverContext->linkStatus));
            HandleEraseBoxMon(sPokeMoverContext->boxView.gscBox, sPokeMoverContext->convertedGSCBoxMons, task->tTransferingMonNum);
            task->tSubState++;
            break;
        case 34:
            result = HandleEraseBoxMon(sPokeMoverContext->boxView.gscBox, NULL, 0);
            if (result < 0) {
                DrawDelayedMessage(0, gTextSaveUnsuccessfully, 180);
                task->tSubState++;
            }
            else if (result == 1) {
                task->tSubState += 2;
                LinkFullSave_Init();
            }
            break;
        case 35:
            if (HandleDelayedMessage(A_BUTTON | B_BUTTON))
                DoSoftReset();
            break;
        case 36:
            if (LinkFullSave_WriteSector()) {
                task->tSubState++;
            }
            break;
        case 37:
            LinkFullSave_ReplaceLastSector();
            FinishSyncErase();
            task->tSubState++;
            break;
        case 38:
            result = HandleEraseBoxMon(sPokeMoverContext->boxView.gscBox, NULL, 0);
            if (result < 0) {
                DrawDelayedMessage(0, gTextSaveUnsuccessfully, 180);
                task->tSubState = 35;
            }
            else if (result == 2) {
                DrawDelayedMessage(0, gTextSaveSuccessfully, 180);
                LinkFullSave_SetLastSectorSignature();
                task->tSubState++;
            }
            break;
        case 39:
            if (HandleDelayedMessage(A_BUTTON))
                return 1;
    }
    return 0;
}

static s32 HandleInfoView(struct Task * task) {
    s8 option;
    switch(task->tSubState) {
        case 0:
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            DrawText(0, gTextAskForInfo, 0, 0, NULL, TRUE);
            task->tSubState++;
            break;
        case 1:
            DrawOptions(sPokeMoverInfoOptions, ARRAY_COUNT(sPokeMoverInfoOptions), &sWindowTemplate_PokeMoverInfoOptions, &task->tWindowId);
            CopyWindowToVram(task->tWindowId, COPYWIN_FULL);
            task->tSubState++;
            break;
        case 2:
            option = Menu_ProcessInput();
            if (option >= 0 && option <= 3) {
                ClearStdWindowAndFrame(task->tWindowId, TRUE);
                RemoveWindow(task->tWindowId);
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                task->tSubState = option + 3;
            }
            else if (option == 4 || JOY_NEW(B_BUTTON)) {
                task->tSubState = 8;
            }
            break;
        case 3:
            DrawMessage(gTextInfoA1, 2);
            task->tSubState = 7;
            break;
        case 4:
            DrawMessage(gTextInfoA2, 2);
            task->tSubState = 7;
            break;
        case 5:
            DrawMessage(gTextInfoA3, 2);
            task->tSubState = 7;
            break;
        case 6:
            DrawMessage(gTextInfoA4, 2);
            task->tSubState = 7;
            break;
        case 7:
            if (HandleMessage())
            {
                task->tSubState = 0;
            }
            break;
        case 8:
            ClearStdWindowAndFrame(task->tWindowId, TRUE);
            RemoveWindow(task->tWindowId);
            return 1;
    }
    return 0;
}

static void Task_HandlePokeMoverMenu(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    int result;
    switch (task->tState)
    {
        case 0:
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            DrawText(0, gMsgAskForNext, 0, 0, NULL, TRUE);
            task->tState++;
            break;
        case 1:
            DrawStartMenu(task->tSelectedOption, &task->tWindowId);
            CopyWindowToVram(task->tWindowId, COPYWIN_FULL);
            task->tState++;
            break;
        case 2:
            task->tSelectedOption = Menu_ProcessInput();
            if (task->tSelectedOption == 0)
            {
                ClearStdWindowAndFrame(task->tWindowId, TRUE);
                RemoveWindow(task->tWindowId);
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                DrawText(0, gMsgAskForConsole, 0, 0, NULL, TRUE);
                task->tState = 3;
            }
            else if (task->tSelectedOption == 1)
            {
                ClearStdWindowAndFrame(task->tWindowId, TRUE);
                RemoveWindow(task->tWindowId);
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                DrawMessage(gTextUsageOfTranfering, 2);
                SetupLinkPokeMover(CB_NotifyStatusChanged);
                task->tState = 8;
            }
            else if (task->tSelectedOption == 2)
            {
                ClearStdWindowAndFrame(task->tWindowId, TRUE);
                RemoveWindow(task->tWindowId);
                task->tState = 10;
                task->tSubState = 0;
            }
            else if (task->tSelectedOption == 3 || JOY_NEW(A_BUTTON))
            {
                task->tState = 11;
            }
            break;
        case 3:
            DrawOptions(sPokeMoverConsoleOptions, ARRAY_COUNT(sPokeMoverConsoleOptions), &sWindowTemplate_PokeMoverConsoleOptions, &task->tWindowId);
            CopyWindowToVram(task->tWindowId, COPYWIN_FULL);
            task->tState++;
            break;
        case 4:
            result = Menu_ProcessInput();
            if (result == 0) {
                ClearStdWindowAndFrame(task->tWindowId, TRUE);
                RemoveWindow(task->tWindowId);
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                DrawMessage(gMsgUsageOfSendingTransferTool, 2);
                task->tState++;
            }
            else if (result == 1) {
                ClearStdWindowAndFrame(task->tWindowId, TRUE);
                RemoveWindow(task->tWindowId);
                task->tState = 7;
                task->tSubState = 0;
            }
            else if (result == 2 || JOY_NEW(B_BUTTON)) {
                ClearStdWindowAndFrame(task->tWindowId, TRUE);
                RemoveWindow(task->tWindowId);
                task->tState = 0;
            }
            break;
        case 5:
            if (HandleMessage())
            {
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                DrawText(0, gTextSending, 0, 0, NULL, TRUE);
                task->tState++;
                task->tSubState = 0;
            }
            break;
        case 6:
            if (HandleSendingTransferToolToGBA(task))
                task->tState = 0;
            break;
        case 7:
            if (HandleSendingTransferToolToGBC(task))
                task->tState = 0;
            break;
        case 8:
            if (HandleMessage())
            {
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                DrawText(0, gTextLinking, 0, 0, NULL, TRUE);
                task->tState++;
                task->tSubState = 0;
            }
            break;
        case 9:
            if (HandleRecvingView(task) != 0)
                task->tState = 0;
            break;
        case 10:
            if (HandleInfoView(task) != 0)
                task->tState = 0;
            break;
        case 11:
            InitSpriteMgr();
            SetBgTilemapBuffer(2, NULL);
            FreeAllWindowBuffers();
            FREE_AND_SET_NULL(sPokeMoverContext);
            ExitLinkPokeMover();
            ResetTasks();
            SetMainCallback2(CB2_ExitPokeMover);
            break;
    }
}

static void Task_SetupPokeMover(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    switch(task->tState)
    {
        case 0:
            SetVBlankHBlankCallbacksToNull();
            task->tState++;
            break;
        case 1:
            ClearScheduledBgCopiesToVram();
            task->tState++;
            break;
        case 2:
            ScanlineEffect_Stop();
            task->tState++;
            break;
        case 3:
            FreeAllSpritePalettes();
            task->tState++;
            break;
        case 4:
            task->tState++;
            break;
        case 5:
            ResetSpriteData();
            task->tState++;
            break;
        case 6:
            SetGpuReg(REG_OFFSET_DISPCNT, 0);
            SetGpuReg(REG_OFFSET_BG3CNT, 0);
            SetGpuReg(REG_OFFSET_BG2CNT, 0);
            SetGpuReg(REG_OFFSET_BG1CNT, 0);
            SetGpuReg(REG_OFFSET_BG0CNT, 0);
            task->tState++;
            break;
        case 7:
            ResetBgsAndClearDma3BusyFlags(0);
            task->tState++;
            break;
        case 8:
            ResetAllBgsCoordinates();
            task->tState++;
            task->tSubState = 0;
            break;
        case 9:
            if (InitBGs(task)) {
                sPokeMoverContext->boxView.state = 0;
                task->tState++;
            }
            break;
        case 10:
            SetVBlankCallback(VBlankCB_PokeMover);
            task->tState++;
            break;
        case 11:
            if (InitScrollableBoxView()) 
                task->tState++;
            break;
        case 12:
            InitTextView();
            task->tState++;
            break;
        case 13:
            LoadPalette(sScrollingBg_Pal, BG_PLTT_ID(3), 32);
            task->tState++;
            break;
        case 14:
            DrawMessage(gMsgWelcomeForPokeMover, 2);
            task->tState++;
            break;
        case 15:
            if (HandleMessage()) {
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                DrawMessage(gMsgInsertCart, 2);
                task->tState++;
            }
            break;
        case 16:
            if (HandleMessage()) {
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                DrawText(0, gTextDetectCart, 0, 0, NULL, TRUE);
                task->tState++;
            }
            break;
        case 17:
            if (DetectRom()) {
                task->tState += 2;
                LoadMonIconPalettes();
            }
            else {
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                DrawDelayedMessage(0, gTextInvaildCart, 180);
                task->tState++;
            }
            break;
        case 18:
            if (HandleDelayedMessage(A_BUTTON | B_BUTTON)) {
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                DrawMessage(gMsgInsertCart, 2);
                task->tState = 16;
            }
            break;
        case 19:
            if (DetectSave())
                task->tState++;
            else {
                FillWindowPixelBuffer(0, PIXEL_FILL(1));
                DrawDelayedMessage(0, gTextInvaildSave, 180);
                task->tState--;
            }
            break;
        case 20:
            task->tState = 0;
            gTasks[taskId].func = Task_HandlePokeMoverMenu;
            break;
    }
}

static void CB2_PokeMover(void)
{
    RunTasks();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
}

static void CB2_ExitPokeMover(void)
{
//    gFieldCallback = FieldTask_ReturnToPcMenu;
//    SetMainCallback2(CB2_ReturnToField);
}

static void VBlankCB_PokeMover(void)
{
    if (sPokeMoverContext != NULL && sPokeMoverContext->bg3Scorllable) {
        ChangeBgX(3, 128, BG_COORD_ADD);
        ChangeBgY(3, 128, BG_COORD_SUB);
    }
    LoadOam();
    ProcessSpriteCopyRequests();
}

void ShowPokeMoverScreen(u8 boxOption)
{
    u8 taskId;
    ResetTasks();
    sPokeMoverContext = (struct PokeMoverContext *)AllocZeroed(sizeof(struct PokeMoverContext));
    memset(sPokeMoverContext->spriteMgr.spriteIds, (s8)-1, sizeof(sPokeMoverContext->spriteMgr.spriteIds));
    taskId =CreateTask(Task_SetupPokeMover, 80);
    gTasks[taskId].tPokeStorageOption = boxOption;
    SetMainCallback2(CB2_PokeMover);
}

static u32 CalcGSCTransferableBoxMonCount(u8 box) {
    struct BoxMonGSCBase *bases = sPokeMoverContext->baseData.gameLang == LANGUAGE_JAPANESE ? 
                                            sPokeMoverContext->baseData.boxes.jp[box] : 
                                            sPokeMoverContext->baseData.boxes.intl[box];
    u32 num = 0;
    u32 boxMonTotal = GET_BOX_MON_TOTAL;
    int i;
    for (i = 0; i < boxMonTotal; i++)
    {
        if (bases[i].species == SPECIES_NONE ||
            bases[i].species > SPECIES_CELEBI ||
            bases[i].hasItem ||
            bases[i].isEgg ||
            (bases[i].species == SPECIES_CELEBI &&
             bases[i].isShiny))
            continue;
        num++;
    }
    return num;
}

static bool8 CheckLocalBoxHasEnoughSpace() {
    u32 transferableCount = CalcGSCTransferableBoxMonCount(sPokeMoverContext->boxView.gscBox);
    u32 localFreeCount = 0;
    u8 localBox = sPokeMoverContext->boxView.currentBox;
    int i;
    for (i = 0; i < IN_BOX_COUNT; i++) {
        if (GetBoxMonData(&gPokemonStoragePtr->boxes[localBox][i], MON_DATA_SPECIES_OR_EGG, NULL) == SPECIES_NONE)
            localFreeCount++;
    }
    return localFreeCount >= transferableCount;
}

static inline struct BoxPokemonGSC * GetGSCBoxMonFromBox(u8 position) {
    if (sPokeMoverContext->baseData.gameLang == LANGUAGE_JAPANESE)
        return &sPokeMoverContext->boxData.jp.monData[position];
    else
        return &sPokeMoverContext->boxData.intl.monData[position];
}

static inline u8 * GetGSCBoxMonOTName(u8 position) {
    if (sPokeMoverContext->baseData.gameLang == LANGUAGE_JAPANESE)
        return sPokeMoverContext->boxData.jp.monOTNames[position];
    else
        return sPokeMoverContext->boxData.intl.monOTNames[position];
}

static inline u8 * GetGSCBoxMonNickName(u8 position) {
    if (sPokeMoverContext->baseData.gameLang == LANGUAGE_JAPANESE)
        return sPokeMoverContext->boxData.jp.monNames[position];
    else
        return sPokeMoverContext->boxData.intl.monNames[position];
}

static s32 DoGSCBoxMonConversion(u8 localBox, struct BoxMonGSCBase * bases, u8 * convertedBoxMons) {
    u32 convertedCounter = 0;
    u8 *otName, otGender,*nickName, total, gameVer, gameLang, *convertedBoxMonsAlt;
    bool8 result;
    struct BoxPokemonGSC *boxMonGSC;
    struct BoxPokemon *boxMon;
    int i, j;
    struct LegalityCheckResult legalityCheckResult;
    sPokeMoverContext->checkResultNum = 0;
    memset(sPokeMoverContext->checkResults, 0, sizeof(sPokeMoverContext->checkResults));
    convertedBoxMonsAlt = convertedBoxMons;
    otGender = sPokeMoverContext->baseData.playerGender;
    gameVer = sPokeMoverContext->baseData.gameVer;
    gameLang = sPokeMoverContext->baseData.gameLang;
    total = sPokeMoverContext->boxData.jp.count;
    for (i = 0; i < total; i++)
    {
        boxMonGSC = GetGSCBoxMonFromBox(i);
        if (boxMonGSC->species != bases[i].species)
            return 0;
    }
    for (i = 0; i < total; i++)
    {
        if (bases[i].species == SPECIES_NONE ||
            bases[i].species > SPECIES_CELEBI ||
            bases[i].hasItem ||
            bases[i].isEgg ||
            (bases[i].species == SPECIES_CELEBI &&
             bases[i].isShiny))
            continue;
        boxMonGSC = GetGSCBoxMonFromBox(i);
        for (j = 0; j < IN_BOX_COUNT; j++)
        {
            boxMon = &gPokemonStoragePtr->boxes[localBox][j];
            if (GetBoxMonData(boxMon, MON_DATA_SPECIES_OR_EGG, NULL) == SPECIES_NONE) {
                legalityCheckResult.boxMonIndex = j;
                if (FindIndexIfGSCBoxMonIsEggless(boxMonGSC) >= 0)
                    result = ConvertEgglessBoxMonFromGSC(boxMon, boxMonGSC, GetGSCBoxMonNickName(i), GetGSCBoxMonOTName(i), otGender, gameVer, gameLang, &legalityCheckResult);
                else
                    result = ConvertBoxMonFromGSC(boxMon, boxMonGSC, GetGSCBoxMonNickName(i), GetGSCBoxMonOTName(i), otGender, gameVer, gameLang, &legalityCheckResult);
                sPokeMoverContext->checkResults[sPokeMoverContext->checkResultNum++] = legalityCheckResult;
                *convertedBoxMons++ = i;
                break;
            }
        }
    }
    return convertedBoxMons - convertedBoxMonsAlt;
}

static void UpdatePokedex() {
    s32 i;
    struct BoxPokemon *mon;
    u16 species;
    u32 personality;
    for (i = 0; i < sPokeMoverContext->checkResultNum; i++) {
        mon = &gPokemonStoragePtr->boxes[sPokeMoverContext->boxView.localBox][sPokeMoverContext->checkResults[i].boxMonIndex];
        species = GetBoxMonData(mon, MON_DATA_SPECIES, NULL);
        personality = GetBoxMonData(mon, MON_DATA_PERSONALITY, NULL);
        species = SpeciesToNationalPokedexNum(species);
        GetSetPokedexFlag(species, FLAG_SET_SEEN);
        HandleSetPokedexFlag(species, FLAG_SET_CAUGHT, personality);
    }
}
