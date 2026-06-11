import os
import re

chinese_char_regex = re.compile(r'[\u4e00-\u9fff]')
unique_chars = set()

files_to_scan = [
    r"src/ui.h",
    r"src/ui_common.c",
    r"src/ui_curve.c",
    r"src/ui_datalog.c",
    r"src/ui_home.c",
    r"src/ui_menu.c",
    r"src/ui_param.c",
    r"src/ui_password.c",
    r"src/ui.c"
]

for filename in files_to_scan:
    filepath = os.path.join(r"f:\my_project_2026\LVGL\lv_port_pc_vscode", filename)
    if os.path.exists(filepath):
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            for char in content:
                if chinese_char_regex.match(char):
                    unique_chars.add(char)

sorted_chars = "".join(sorted(list(unique_chars)))
print("Unique Chinese characters:", sorted_chars)
print("Count:", len(sorted_chars))

with open("unique_chars.txt", "w", encoding="utf-8") as f:
    f.write(sorted_chars)
