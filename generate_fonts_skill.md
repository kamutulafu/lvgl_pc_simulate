# Skill: LVGL CJK Font Generation (Safe Windows UTF-8 Flow)

This skill describes the procedure to generate custom CJK fonts (`ui_font_CJK_14` and `ui_font_CJK_24`) for LVGL without encoding corruption on Windows, including automatic post-processing for LVGL v9 compatibility.

## 1. Background
When running font generation tools like `lv_font_conv` from Python's subprocess or terminal directly on Windows, the OS/command-line argument parser encodes strings using the system active code page (usually CP936/GBK for Simplified Chinese). This corrupts UTF-8 CJK strings, leading to missing glyphs rendering as square box artifacts in the UI.

To prevent this, the generation is performed directly via Node.js in-process by loading `lv_font_conv_cli` API, guaranteeing lossless UTF-8 representation.

---

## 2. Font Generation Script Setup
The font builder is implemented in [generate_fonts.js](generate_fonts.js). It defines:
- **Font sizes**: 14px (`ui_font_CJK_14`) and 24px (`ui_font_CJK_24`).
- **Source fonts**: `SourceHanSansSC-Normal.otf` (Chinese) and `FontAwesome5-Solid+Brands+Regular.woff` (icons).
- **Symbols list**: The exact CJK character string required by the UI.

---

## 3. How to Execute Font Generation

To (re)generate the font files:

1. Open PowerShell or cmd in the workspace root directory.
2. Run the following command:
   ```bash
   node generate_fonts.js
   ```
3. Verify the output:
   - `src/fonts/ui_font_CJK_14.c`
   - `src/fonts/ui_font_CJK_24.c`
   - Post-processing logs confirming `.cap_height` and `.x_height` were commented out.

---

## 4. How to Customize CJK Characters

If you add new Chinese characters to the UI:

1. Open [generate_fonts.js](generate_fonts.js).
2. Locate the `symbols` string constant at the top:
   ```javascript
   const symbols = "°·–—₂₄...一上下与...（），：";
   ```
3. Append your new Chinese characters to the string.
4. Save the file (ensure your editor saves it as **UTF-8**).
5. Re-run `node generate_fonts.js` and rebuild your project.

---

## 5. Rebuilding the Project
After generating the fonts, compile the changes in the build directory:
```bash
cmake --build ./build --config Debug --target all -j 8 --
```
