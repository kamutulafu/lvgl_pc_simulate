const path = require('path');
const fs = require('fs');
const cli = require('f:/my_project_2026/LVGL/lv_font_conv_cli/lib/cli');

let symbols = "°·–—₂₄℃−▲▼●（），：？◀▶";
try {
    symbols += fs.readFileSync(path.join(__dirname, "unique_chars.txt"), "utf8");
} catch (e) {
    console.error("Failed to read unique_chars.txt:", e);
}

const cjk_14_args = [
    "--font",
    "f:/my_project_2026/LVGL/lv_port_pc_vscode/lvgl/scripts/built_in_font/SourceHanSansSC-Normal.otf",
    "--range",
    "0x20-0x7f",
    "--symbols",
    symbols,
    "--font",
    "f:/my_project_2026/LVGL/lv_port_pc_vscode/lvgl/scripts/built_in_font/FontAwesome5-Solid+Brands+Regular.woff",
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
    "src/fonts/ui_font_CJK_14.c",
];

const cjk_24_args = [
    "--no-compress",
    "--no-prefilter",
    "--bpp",
    "4",
    "--size",
    "24",
    "--font",
    "f:/my_project_2026/LVGL/lv_port_pc_vscode/lvgl/scripts/built_in_font/SourceHanSansSC-Normal.otf",
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
    "src/fonts/ui_font_CJK_24.c",
];

function postProcess(filepath) {
    if (!fs.existsSync(filepath)) return;
    console.log(`Post-processing ${filepath}...`);
    let content = fs.readFileSync(filepath, 'utf8');
    content = content.replace(/(\s*)(\.cap_height\s*=\s*\d+,)/g, "$1/* $2 */");
    content = content.replace(/(\s*)(\.x_height\s*=\s*\d+,)/g, "$1/* $2 */");
    fs.writeFileSync(filepath, content, 'utf8');
    console.log(`Successfully processed ${filepath}`);
}

async function run() {
    try {
        console.log("Generating CJK_14...");
        await cli.run(cjk_14_args);
        console.log("CJK_14 generated successfully.");
        postProcess("src/fonts/ui_font_CJK_14.c");

        console.log("Generating CJK_24...");
        await cli.run(cjk_24_args);
        console.log("CJK_24 generated successfully.");
        postProcess("src/fonts/ui_font_CJK_24.c");
    } catch (err) {
        console.error(err);
        process.exit(1);
    }
}

run();
