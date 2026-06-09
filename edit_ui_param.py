import os

with open('src/ui_param.c', 'r', encoding='utf-8') as f:
    code = f.read()

target = """    if (dig_lbl_hint) {
        if (confirm) {
            char hb[32];
            int off = snprintf(hb, sizeof(hb), "%s", ui_get_text("值: "));
            for (int i = 0; i < (int)p->digit_count && off < (int)sizeof(hb); i++)
                off += snprintf(hb + off, sizeof(hb) - off, "%d", g_edit_dig[i]);
            lv_label_set_text(dig_lbl_hint, hb);
        } else {
            const char *ht = sel_mode ? ui_get_text("UP/DOWN 选择位   OK 进入编辑该位")
                                       : ui_get_text("UP/DOWN 修改当前位数字   OK 移到下一位");
            lv_label_set_text(dig_lbl_hint, ht);
        }
    }"""

replacement = """    if (dig_lbl_hint) {
        if (confirm) {
            char hb[32];
            int off = snprintf(hb, sizeof(hb), "%s", ui_get_text("值: "));
            for (int i = 0; i < (int)p->digit_count && off < (int)sizeof(hb); i++)
                off += snprintf(hb + off, sizeof(hb) - off, "%d", g_edit_dig[i]);
            lv_label_set_text(dig_lbl_hint, hb);
            lv_obj_set_style_text_color(dig_lbl_hint, C_WARN, 0);
        } else {
            const char *ht = sel_mode ? ui_get_text("UP/DOWN 选择位   OK 进入编辑该位")
                                       : ui_get_text("UP/DOWN 修改当前位数字   OK 移到下一位");
            lv_label_set_text(dig_lbl_hint, ht);
            lv_obj_set_style_text_color(dig_lbl_hint, lv_color_hex(0x88AACC), 0);
        }
    }"""

code_norm = code.replace('\r\n', '\n')
target_norm = target.replace('\r\n', '\n')
replacement_norm = replacement.replace('\r\n', '\n')

if target_norm in code_norm:
    new_code = code_norm.replace(target_norm, replacement_norm)
    with open('src/ui_param.c', 'w', encoding='utf-8', newline='\r\n') as f:
        f.write(new_code)
    print("Success")
else:
    print("Failed to find target")
