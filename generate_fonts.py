import subprocess
import os
import re

symbols = "°·–—₂₄℃−▲▼●一上下与两个中串二亮仪传位低体作保信修值光入写准减出创到前剩功势化单原参取右号名周器囊回围型增备大头子字存定实密射局层左已布常序度延建弃式当录待徽态息意感成报择持按换据描提操支改放数整文明映最期机条标栏校样格检模次正步气氢氧氮注测浏消点烷状甲界码硫确碳示种移符等签箭类系统继续编置背胶范菜行表装览言警认记设访证该语误说请调趋辑输运返还进连迟述退选逐递通道采量锁错键问阈限面页顶饰验高（），："

cjk_14_args = [
    "node",
    r"f:\my_project_2026\LVGL\lv_font_conv_cli\lv_font_conv.js",
    "--font",
    r"f:\my_project_2026\LVGL\lv_port_pc_vscode\lvgl\scripts\built_in_font\SourceHanSansSC-Normal.otf",
    "--range",
    "0x20-0x7f",
    "--symbols",
    symbols,
    "--font",
    r"f:\my_project_2026\LVGL\lv_port_pc_vscode\lvgl\scripts\built_in_font\FontAwesome5-Solid+Brands+Regular.woff",
    "--range",
    "61441,61448,61451,61452,61453,61457,61459,61461,61465,61468,61473,61478,61479,61480,61502,61507,61512,61515,61516,61517,61521,61522,61523,61524,61543,61544,61550,61552,61553,61556,61559,61560,61561,61563,61587,61589,61636,61637,61639,61641,61664,61671,61674,61683,61724,61732,61787,61931,62016,62017,62018,62019,62020,62087,62099,62212,62189,62810,63426,63650",
    "--size",
    "14",
    "--format",
    "lvgl",
    "--bpp",
    "4",
    "--no-compress",
    "--no-prefilter",
    "--force-fast-kern-format",
    "--lv-font-name",
    "ui_font_CJK_14",
    "--lv-include",
    "../ui.h",
    "-o",
    r"src\fonts\ui_font_CJK_14.c",
]

cjk_24_args = [
    "node",
    r"f:\my_project_2026\LVGL\lv_font_conv_cli\lv_font_conv.js",
    "--no-compress",
    "--no-prefilter",
    "--bpp",
    "4",
    "--size",
    "24",
    "--font",
    r"f:\my_project_2026\LVGL\lv_port_pc_vscode\lvgl\scripts\built_in_font\SourceHanSansSC-Normal.otf",
    "--range",
    "0x20-0x7f",
    "--symbols",
    symbols,
    "--format",
    "lvgl",
    "--lv-font-name",
    "ui_font_CJK_24",
    "--lv-include",
    "../ui.h",
    "--force-fast-kern-format",
    "-o",
    r"src\fonts\ui_font_CJK_24.c",
]

def post_process_font(filepath):
    if not os.path.exists(filepath):
        return
    print(f"Post-processing {filepath} to comment out LVGL v9 unsupported members...")
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()
    
    # Comment out .cap_height and .x_height initializers
    content = re.sub(r"(\s*)(\.cap_height\s*=\s*\d+,)", r"\1/* \2 */", content)
    content = re.sub(r"(\s*)(\.x_height\s*=\s*\d+,)", r"\1/* \2 */", content)
    
    with open(filepath, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"Successfully processed {filepath}")

print("Generating CJK_14 font...")
res1 = subprocess.run(cjk_14_args, capture_output=True, text=True, encoding="utf-8")
if res1.returncode != 0:
    print("Error generating CJK_14:")
    print(res1.stderr)
else:
    print("CJK_14 font generated successfully.")
    post_process_font(r"src\fonts\ui_font_CJK_14.c")

print("Generating CJK_24 font...")
res2 = subprocess.run(cjk_24_args, capture_output=True, text=True, encoding="utf-8")
if res2.returncode != 0:
    print("Error generating CJK_24:")
    print(res2.stderr)
else:
    print("CJK_24 font generated successfully.")
    post_process_font(r"src\fonts\ui_font_CJK_24.c")

