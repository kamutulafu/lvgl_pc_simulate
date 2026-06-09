import os
import re

# Regex to find Chinese characters
chinese_char_regex = re.compile(r'[\u4e00-\u9fff]')

unique_chars = set()

# Scan src/ directory
src_dir = r"f:\my_project_2026\LVGL\lv_port_pc_vscode\src"
for root, dirs, files in os.walk(src_dir):
    for file in files:
        if file.endswith(('.c', '.h')):
            filepath = os.path.join(root, file)
            # Skip CJK font files since they contain all character comments
            if "ui_font_CJK" in file:
                continue
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                for char in content:
                    if chinese_char_regex.match(char):
                        unique_chars.add(char)

# Write to unique_chars.txt with UTF-8
sorted_chars = "".join(sorted(list(unique_chars)))
with open("unique_chars.txt", "w", encoding="utf-8") as f:
    f.write(sorted_chars)

print(f"Total count of unique Chinese characters: {len(sorted_chars)}")
