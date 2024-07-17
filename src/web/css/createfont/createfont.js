#! /usr/bin/env node
/*  Usage: node ./createfont.js
    此脚本用于将 icomoon.ttf 字体文件中的图标替换为自定义的 svg 文件，并生成新的字体文件。
    目前使用的 summernote 富文本编辑器的字体图标是 icomoon.ttf，因此需要将其中的图标替换
    为自定义的 svg 文件。
    在 style.css 文件中设置 font-size 等属性调整界面。

    脚本使用 font-carrier 库，具体使用方法请参考：
    sa: https://github.com/purplebamboo/font-carrier
 */

// 字体文件路径
var inputFile = '../fonts/icomoon.ttf'
var outputFile = '../fonts/icomoon'

// 引入 font-carrier 库
var fontCarrier = require('font-carrier')
var font = fontCarrier.transfer(inputFile)
const fs = require('fs')

// 追加 svg 图片和字体
function setGlyph(_glyph, _glyphName, _filePath) {
    var svgFile = fs.readFileSync(_filePath).toString();
    font.setGlyph(_glyph, {
        svg: svgFile,
        glyphName: _glyphName
    })
}

// 设置字体图标 
setGlyph('&#xe903;', 'marker', './svg/marker.svg')
setGlyph('&#xe904;', 'text_color_outline', './svg/text_color_outline.svg')
setGlyph('&#xe905;', 'text_color_fill', './svg/text_color_fill.svg')
setGlyph('&#xe906;', 'bold', './svg/bold.svg')
setGlyph('&#xe907;', 'richtext_arrow', './svg/richtext_arrow.svg')
setGlyph('&#xe908;', 'italic', './svg/Italic.svg')
setGlyph('&#xe909;', 'bullet_number', './svg/bullet_number.svg')
setGlyph('&#xe90a;', 'bullet_dot', './svg/bullet_dot.svg')
setGlyph('&#xe90b;', 'strikethrough', './svg/Strikethrough.svg')
setGlyph('&#xe90c;', 'underline', './svg/underline.svg')
setGlyph('&#xe90f;', 'text_color', './svg/text_color.svg')

// 输出新字体文件
font.output({
    path: outputFile
})