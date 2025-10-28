//C++ 调用js接口
//信号绑定
// initData(const QString& jsonData); 初始化，参数为json字符串
// void setHtml(const QString& html); 初始化，设置html
// insertVoiceItem(const QString &jsonData);　插入语音，参数为json字符串

//callback回调
// const QString getHtml();获取整个html
// const QString getAllNote();获取所有语音列表的Json
//

// 注册右键点击事件
$('body').on('contextmenu', rightClick)
// 注册内容改变事件
$('#summernote').on('summernote.change', changeContent);

// 初始化渲染模板
var h5Tpl = `
    <div class="li voiceBox" contenteditable="false" jsonKey="{{jsonValue}}">
        <div class='voiceInfoBox'>

        <!-- 进度条teste -->
            <div class="demo" >              
                <div class="voicebtn play"></div>
                <div class="lf">
                    <div class="title">{{title}}</div>
                    <div class="minute padtop">{{createTime}}</div>
                </div>
                <div class="lr">
                    <div class="icon">
                        <div class="wifi-symbol">
                            <div class="wifi-circle"></div>
                        </div>
                    </div>
                    <div class="time padtop">{{transSize}}</div>
                </div>
            </div>
            <div class="translate">
            </div>
        </div>
    </div>
    {{#if text}}
    <p>{{text}}</p>
    {{/if}}
    `;

// 语音插入模板
var nodeTpl = `
        <div class='voiceInfoBox'>
            <div class="demo"  >
                <div class="voicebtn play"></div>
                <div class="lf">
                    <div class="title">{{title}}</div>
                    <div class="minute padtop">{{createTime}}</div>
                </div>
                <div class="lr">
                    <div class="icon">
                        <div class="wifi-symbol">
                            <div class="wifi-circle"></div>
                        </div>
                    </div>
                    <div class="time padtop">{{transSize}}</div>
                </div>
            </div>
            <div class="translate">
            </div>
        </div>`;

// 语言播放模板
var voicePlayBackTemplate = `
    <!-- 语音播放控件 -->
    <div class="voicePlayback">
        <div class="left">
            <div class="voiceBtn"></div>
            <div class="title">{{title}}</div>
        </div>
        <!-- 音频进度条 音频 step 单位 s ; range 单位 ms -->
        <input class="progressBar" type="range" min="0" max="{{voiceSize}}" step="1000" value="0"
            onchange="onProgressChange()" oninput="onProgressInput()">
        <div class="right">
            <!-- 语音进度/长度字段，默认显示长度 -->
            <div class="timeField">
                <div class="timePassed">00:00/</div>
                <div class="timeTotal">{{transSize}}</div>
            </div>
            <!-- 语音转文字 -->
            <div class="voiceToTextLabel">
                <div class="voiceToTextIcon"></div>
                <div class="translatingLabel">{{translatingLabel}}</div>
            </div>
            <div class="closePlaybackBarBtn"></div>
        </div>
    </div>`;

// 语音转文本模板 
// noAirPopover 用于框选此 div 内容时不显示浮动编辑工具栏
var translateTextContentTemplate = ` 
    <div class="translateHeader">
        <div class="translateIcon"></div>
        <div class="translateLabel">{{translateLabel}}</div>
        <div class="foldBtn"></div>
    </div>
    <div class="translateText noAirPopover">{{translateText}}</div>
    `;

// 悬浮语音播放控件，文档唯一
var airVoicePlayBackTemplate = `
    <div class="airVoicePlayback" id="airVoicePlayback">
        ${voicePlayBackTemplate}
    </div>`;

var nodeTplV2 = `
    <!-- 语音播放/转文本控件 Version2 -->
    <div class='voiceInfoBox version2'>
        ${voicePlayBackTemplate}
        <!-- 语音转文字显示控件 -->
        <div class="translate">
        </div>
    </div>`;

var h5TplV2 = `
    <div class="li voiceBox" draggable="true" jsonKey="{{jsonValue}}">
        ${nodeTplV2}
    </div>
    {{#if text}}
    <p>{{text}}</p>
    {{/if}}
    `;

var formatHtml = ''
var webobj;    //js与qt通信对象
var progressBarDraging = false; // 进度条拖动状态
var activePlayback = null;  //当前播放的语音对象
var airVoicePlayback = null;  //悬浮播放对象
var activeTransVoice = null;  //执行语音转文字对象
var bTransVoiceIsReady = true;  //语音转文字是否准备好
var initFinish = false;
var isVoicePaste = false
var isShowAir = true
var nowClickVoice = null
var global_activeColor = ''
var global_disableColor = ''
var global_theme = 1    // theme type 1:light 2:dark
var global_themeColor = 'transparent'  //主题色
var global_isRecording = false  // 录音状态标志
var scrollHide = null  //滚动条隐藏定时器
var scrollHideFont = null  //字体滚动条定时
var isUlOrOl = false
const airPopoverHeight = 44  //悬浮工具栏高度
const airPopoverWidth = 385  //悬浮工具栏宽度
const airPopoverLeftMargin = 20 //悬浮工具栏距离编辑区左侧边距
const airPopoverRightMargin = 35 //悬浮工具栏距离编辑区右侧边距

// 翻译列表
var tooltipContent = {
    fontname: '字体',
    fontsize: '字号',
    forecolor: '字体颜色',
    backcolor: '背景色',
    bold: '加粗',
    italic: '斜体',
    underline: '下划线',
    strikethrough: '删除线',
    ul: '无序列表',
    ol: '有序列表',
    more: '更多颜色',
    recentlyUsed: '最近使用'
}
// 动态翻译内容
var divTranslateContent = {
    translatingLabel: '正在转为文字',
    translateLabel: '语音转文字'
}

// 字体列表
var global_fontList = []

// 国际化
function changeLang(tooltipContent) {
    for (let item in tooltipContent) {
        if (item != 'recentlyUsed') {
            $(`.${item}But`).attr('data-original-title', tooltipContent[item])
        } else {
            $('.recentlyUsedBut').html(tooltipContent[item])
        }
    }
}

function changeDivTextLang(divTranslateContent) {
    for (let item in divTranslateContent) {
        $(`${item}`).html(divTranslateContent[item]);
    }
}

/**
 * 设置初始化字体
 * @date 2022-04-12
 * @param {string} initFontName 字体名
 * @returns {any}
 */
function setInitFont(initFontName) {
    $('body').css('font-family', initFontName)
}

// 建立通信
new QWebChannel(qt.webChannelTransport,
    function (channel) {
        webobj = channel.objects.webobj;
        //所有的c++ 调用js的接口都需要在此绑定格式，webobj.c++函数名（jscontent.cpp查看.connect(js处理函数)
        //例如 webobj.c++fun.connect(jsfun)
        webobj.callJsInitData.connect(initData);
        webobj.callJsInsertVoice.connect(insertVoiceItem);
        webobj.callJsSetPlayStatus.connect(toggleState);
        webobj.callJsSetHtml.connect(setHtml);
        webobj.callJsSetVoiceText.connect(setVoiceText);
        webobj.callJsInsertImages.connect(insertImg);
        webobj.callJsSetTheme.connect(changeColor);
        webobj.calllJsShowEditToolbar.connect(showRightMenu);
        webobj.callJsHideEditToolbar.connect(hideRightMenu);
        webobj.callJsClipboardDataChanged.connect(shearPlateChange);
        webobj.callJsSetVoicePlayBtnEnable.connect(playButColor);
        webobj.callJsSetFontList.connect(setFontList);

        webobj.callJsVoicePlayProgressChanged.connect(updateProgressBar);
        webobj.callJsDeleteSelection.connect(deleteSelection);
        //通知QT层完成通信绑定
        webobj.jsCallChannleFinish();
        // setFontList(global_fontList, "Unifont")
    }
)

// 获取字体列表并初始化
function setFontList(fontList, initFont) {
    global_fontList = fontList;
    setInitFont(initFont)
    initSummernote()
    // 通知QT，summernote初始化完成
    webobj.jsCallSummernoteInitFinish()
    // 获取翻译和字体列表后，再初始化summernote
    webobj.jsCallGetTranslation(function (res) {
        changeLang(JSON.parse(res))
    })
    webobj.jsCallDivTextTranslation(function (res) {
        divTranslateContent = JSON.parse(res)
        changeDivTextLang(divTranslateContent)
    })
}

// 初始化summernote
function initSummernote() {
    $('#summernote').summernote({
        focus: true,
        disableDragAndDrop: true,
        lang: 'zh-CN',
        // 浮动工具栏
        popover: {
            air: [
                ['fontname', ['fontname']],
                ['fontsize', ['fontsize']],
                ['line'],
                ['forecolor', ['forecolor']],
                ['backcolor', ['backcolor']],
                ['bold', ['bold']],
                ['italic', ['italic']],
                ['underline', ['underline']],
                ['strikethrough', ['strikethrough']],
                ['line'],
                ['ul', ['ul']],
                ['ol', ['ol']],
            ],
        },
        fontNames: global_fontList,
        airMode: true,
    });
    // 注册滚动事件
    listenFontnameList()
    // 默认选中前景色和背景色
    setSelectColorButton($('[data-value="#000000"]'))
    setSelectColorButton($('[data-value="#FFFFFF33"]'))
    var selectColor = $('.note-backcolor .selectColor').css('background-color')
    $('.icon-backcolor .path1').css('color', selectColor)

    // 监听窗口大小变化
    $(window).resize(function () {
        $('.note-editable').css('min-height', $(window).height())
    }).resize();

    // 添加悬浮语音播放控件
    addAirVoicePlayback();
    // 初始化录音块拖拽（仅交换数据与模板，不让浏览器复制内联样式）
    initVoiceDragAndDrop();
    // 移除 Summernote 的覆盖式 dropzone，避免拦截 drop 事件
    $('.note-dropzone').remove();
}




/**
 * 通知后台存储页面内容
 * @date 2021-08-19
 * @param {any} we
 * @param {any} contents
 * @param {any} $editable
 * @returns {any}
 */
function changeContent(we, contents, $editable) {
    if ($('.note-editable').html() == '') {
        $('.note-editable').html('<p><br></p>')
    }
    if (webobj && initFinish) {
        if (!$('.note-air-popover').is(':hidden')) {
            if (getSelectedRange().innerHTML == "") {
                $('#summernote').summernote('airPopover.hide')
            }
        }
        // 清理撤销后可能残留的临时CSS状态类
        // summernote的undo可能恢复了包含'now'、'play'等临时状态的HTML
        $('.voicePlayback').each(function() {
            var $playback = $(this);           
            if (activePlayback === null || 
                !activePlayback.length || 
                !$playback.is(activePlayback) ||
                !$.contains(document.documentElement, activePlayback[0])) {
                $playback.removeClass('now').removeClass('play').removeClass('pause');
            }
        });
        
        webobj.jsCallTxtChange();
    }
}

/**
 * 处理编辑页面大小改变事件
 * @date 2023-05-30
 * @param {any}
 * @returns {any}
 */
window.onresize = function () {
    updateAirPopoverPos()
}

/**
 * 处理air-popover位置更新
 * @date 2023-05-30
 * @param {any} 
 * @returns {any}
 */
function updateAirPopoverPos() {
    if (!$('.note-air-popover').is(':hidden')) {
        if (getSelectedRange().innerHTML != "") {
            $('#summernote').summernote('airPopover.update')
        }
    }
}

// 判断编辑区是否为空
function isNoteNull() {
    return $('.note-editable').html() === '<p><br></p>'
}

//点击选中录音
$('body').on('click', '.li', function (e) {
    // 如果点击的是 translateText，不选中语音块，让用户选择文本
    if ($(e.target).closest('.translateText').length > 0) {
        return;
    }
    
    e.stopPropagation();
    $('.li').removeClass('active');
    setSelectRange(this)
    isShowAir = false
    $(this).addClass('active');
})

$('body').on('click', '.translate', function (e) {
    // 阻止冒泡
    e.stopPropagation();
})

// voiceBox 和 translateText 阻止编辑但允许选择
$('body').on('keydown keypress input', '.voiceBox, .translateText', function (e) {
    // 只允许选择和复制，阻止所有编辑
    if ((e.ctrlKey || e.metaKey) && (e.keyCode === 67 || e.keyCode === 65)) {
        // Ctrl+C / Ctrl+A 允许
        return true;
    }
    e.preventDefault();
    return false;
})

$('body').on('paste drop', '.voiceBox, .translateText', function (e) {
    e.preventDefault();
    return false;
})

// translateText 的鼠标和拖拽事件：禁止拖拽整个 voiceBox，允许文本选择
$('body').on('mousedown', '.translateText', function (e) {
    // 阻止冒泡到 .li，避免选中整个语音块
    e.stopPropagation();
    
    // 临时禁用父元素的 draggable 属性，让文本可以被选择
    var $voiceBox = $(this).closest('.voiceBox');
    $voiceBox.attr('draggable', 'false');
})

// 鼠标释放时恢复 draggable
$('body').on('mouseup', '.translateText', function (e) {
    var $voiceBox = $(this).closest('.voiceBox');
    $voiceBox.attr('draggable', 'true');
})

// 阻止 translateText 触发 dragstart（防止拖拽整个语音块）
$('body').on('dragstart', '.translateText', function (e) {
    e.preventDefault();
    e.stopPropagation();
    return false;
})

/**
 * 获取选区
 * @date 2021-09-08
 * @returns {any} div
 */
function getSelectedRange() {
    var selectionObj = window.getSelection();
    var rangeObj = selectionObj.getRangeAt(0);
    var docFragment = rangeObj.cloneContents();
    var testDiv = document.createElement("div");
    testDiv.appendChild(docFragment)
    return testDiv
}

/**
 * 设置光标位置
 * @date 2021-09-09
 * @param {any} element 元素
 * @param {any} pos 起始偏移量
 * @returns {any}
 */
function setFocus(element, pos) {
    var range, selection;
    range = document.createRange();
    range.selectNodeContents(element);
    if (element.innerHTML.length > 0) {
        range.setStart(element.childNodes[0], pos);
    }
    range.collapse(true);       //设置选中区域为一个点
    selection = window.getSelection();
    selection.removeAllRanges();
    selection.addRange(range);
}

/**
 * 设置选区
 * @date 2021-09-10
 * @param {any} dom
 * @returns {any}
 */
function setSelectRange(dom) {
    var sel = window.getSelection();
    sel.removeAllRanges();
    var range = document.createRange();
    range.selectNode(dom);
    // range.collapse(true);
    if (sel.anchorOffset == 0) {
        sel.addRange(range);
    };

}

/**
 * 移除选区
 * @date 2021-09-10
 * @param {any} dom
 * @returns {any}
 */
function removeSelectRange(dom) {
    var sel = window.getSelection();
    var range = document.createRange();
    range.selectNode(dom);
    sel.removeRange(range)
}

// 取消选中样式
$('body').on('click', function (e) {
    $('.li').removeClass('active');
    if ($(e.target).closest('.voiceBox').length == 0 && 
        $(e.target).closest('.voicePlayback').length == 0 &&
        !$(e.target).hasClass('demo') && 
        $(e.target).parents('.demo').length == 0) {
        try {
            var sel = window.getSelection();
            if (sel.rangeCount > 0) {
                var range = sel.getRangeAt(0);
                var fragment = range.cloneContents();
                if ($(fragment).find('.voiceBox').length > 0) {
                    sel.removeAllRanges();
                }
            }
        } catch (e) {
        }
    }
})

// 语音复制
document.addEventListener('copy', copyVoice);
// 语音剪切
document.addEventListener('cut', copyVoice);




// 语音复制功能
function copyVoice(event) {
    isVoicePaste = false
    isUlOrOl = false
    var selectionObj = window.getSelection();
    var rangeObj = selectionObj.getRangeAt(0);
    var docFragment = rangeObj.cloneContents();
    var testDiv = document.createElement("div");

    // 判断是否语音复制
    if ($(docFragment).find('.voiceBox').length != 0) {
        $(docFragment).find('.translate').html('')
        $(docFragment).find('.voiceBox').removeClass('active')
        // V1
        $(docFragment).find('.voicebtn').removeClass('pause').addClass('play');
        $(docFragment).find('.voicebtn').removeClass('now');
        $(docFragment).find('.wifi-circle').removeClass('first').removeClass('second').removeClass('third').removeClass('four').removeClass('fifth').removeClass('sixth').removeClass('seventh');
        // V2
        $(docFragment).find('.voicePlayback').removeClass('now').removeClass('play').removeClass('pause').removeClass('voiceToText');
        $(docFragment).find('.voiceInfoBox').removeClass('containText');

        isVoicePaste = true

    }
    testDiv.appendChild(docFragment);
    isUlOrOl = $(testDiv).children().toArray().every(item => {
        return item.tagName == 'LI'
    })
    // 有序无序列表复制
    if (isUlOrOl) {
        let tagName = rangeObj.commonAncestorContainer.tagName
        let box = document.createElement(tagName)
        docFragment = rangeObj.cloneContents();
        box.appendChild(docFragment)
        $(box).find('p').toArray().forEach(item => {
            if (item.innerHTML == '') {
                item.innerHTML = '<br />'
            }
        })
        testDiv.innerHTML = ''
        testDiv.appendChild(box)
    }
    formatHtml = testDiv.innerHTML;
    if ($(testDiv).children().length == 1 && $(testDiv).children()[0].tagName != 'UL' && $(testDiv).find('.voiceBox').length != 0) {
        formatHtml = '<p><br></p>' + formatHtml

    }
    webobj.jsCallSetClipData(selectionObj.toString().replace(/\n\n/g, '\n'), formatHtml)
    if (event.type == 'cut') {
        // 记录之前数据
        $('#summernote').summernote('editor.recordUndo')
        document.execCommand('delete', false);
        // 主动触发change事件
        changeContent()
    }
    event.preventDefault()
}

// 剪切板内容变化
function shearPlateChange() {
    isVoicePaste = false
}

// 复制标志
function returnCopyFlag() {
    return isVoicePaste
}

// 粘贴
document.addEventListener('paste', function (event) {
    if (formatHtml != "" && isVoicePaste) {
        document.execCommand('insertHTML', false, formatHtml + "<p><br></p>");
        event.preventDefault()
    }
    setFocusScroll()
    removeNullP()
});

// 页面滚动到光标位置
function setFocusScroll() {
    let focusY = getCursortPosition(document.querySelector('.note-editable'))
    let scrollTop = $(document).scrollTop()
    let viewY = $(window).height() + scrollTop
    if (focusY < scrollTop || focusY > viewY) {
        $(document).scrollTop(focusY - $(window).height())
    }
}

// 记录当前选中语音
function recordActiveVoice() {
    var selectionObj = window.getSelection();
    var rangeObj = selectionObj.getRangeAt(0);
    let div = document.createElement('div')
    rangeObj.insertNode(div)
    let $voice = $(div).next()
    $voice.closest('.li').addClass('active')
    if (bTransVoiceIsReady) {
        activeTransVoice = $voice.closest('.li');
    }
    $('#summernote').summernote('airPopover.hide')
    setSelectRange($voice.closest('.voiceBox')[0])
    $(div).remove()
}


/**
 * 判断选区类别
 * @date 2021-09-06
 * @returns {object} 0图片 1语音 2文本
 */
function isRangeVoice() {
    let selectedRange = {
        flag: null,
        info: ''
    }
    var testDiv = getSelectedRange();
    let childrenLength = $(testDiv).children().length
    let voiceLength = $(testDiv).find('.voiceBox').length

    if (voiceLength == childrenLength && childrenLength != 0) {
        selectedRange.flag = 1
        selectedRange.info = $(testDiv).find('.voiceBox:first').attr('jsonKey')
        recordActiveVoice()
    } else if ($(testDiv).find('img').length == childrenLength
        && childrenLength == 1
        && $(testDiv).find('img').parent()[0].childNodes.length == 1) {
        selectedRange.flag = 0
        selectedRange.info = $(testDiv).find('img').attr('src')
    } else {
        selectedRange.flag = 2
    }
    return selectedRange
}

/**
 * 播放按钮点击时触发
 */
$('body').on('click', '.voiceBtn', function (e) {
    // 录音时禁止播放语音
    if (global_isRecording) {
        console.log("Cannot play voice while recording");
        return;
    }
    
    var curPlayback = $(this).parents('.voicePlayback:first');
    // 点击浮动窗口时视同点击焦点音频播放控件
    if (curPlayback.is(airVoicePlayback)) {
        curPlayback = activePlayback;
    }
    var curVoice = $(this).parents('.li:first');
    var jsonString = curVoice.attr('jsonKey');
    var bIsSame = curPlayback.hasClass('now');
    var isActivePlaybackValid = (null !== activePlayback && 
                                  activePlayback.length > 0 && 
                                  $.contains(document.documentElement, activePlayback[0]));
    
    if (!isActivePlaybackValid && activePlayback !== null) {
        activePlayback = null;
        bIsSame = false; 
    }

    // 不同，更新当前播放控件；暂停设置等待后端处理完成后 -> callJsSetPlayStatus()
    if (!bIsSame) {
        if (null !== activePlayback && activePlayback.hasClass('now')) {
            // 移除之前控件的状态，重置样式
            activePlayback.removeClass('now').removeClass('play').removeClass('pause');
        }
        curPlayback.addClass('now');
        activePlayback = curPlayback;
        // 重置悬浮工具栏状态
        resetAirVoicePlayback();
    }

    // 后端播放处理，等待后端开始播放音频数据 -> callJsSetPlayStatus()
    webobj.jsCallPlayVoice(jsonString, bIsSame, function (state) {
        //TODO 录音错误处理
    });
})

//获取整个处理后Html串,去除所有标签中临时状态
function getHtml() {
    var $cloneCode = $('.note-editable').clone();
    $cloneCode.find('.li').removeClass('active');
    $cloneCode.find('.voicebtn').removeClass('pause').addClass('play');
    $cloneCode.find('.voicebtn').removeClass('now');
    $cloneCode.find('.wifi-circle').removeClass('first').removeClass('second').removeClass('third').removeClass('four').removeClass('fifth').removeClass('sixth').removeClass('seventh');
    $cloneCode.find('.translate').html("");
    // V2
    $cloneCode.find('.voicePlayback').removeClass('now').removeClass('play').removeClass('pause').removeClass('voiceToText');
    $cloneCode.find('.voiceInfoBox').removeClass('containText');
    // 移除悬浮工具栏
    $cloneCode.find('.airVoicePlayback').removeClass('active').html("");
    return $cloneCode[0].innerHTML;
}

//获取当前所有的语音列表
function getAllNote() {
    var jsonObj = {};
    var jsonArray = [];
    var jsonString;
    $('.li').each(function () {
        jsonString = $(this).attr('jsonKey');
        jsonArray[jsonArray.length] = JSON.parse(jsonString);
    })
    jsonObj.noteDatas = jsonArray;
    var retJson = JSON.stringify(jsonObj);
    return retJson;
}

//获取当前选中录音json串
function getActiveNote() {
    var retJson = '';
    if ($('.active').length > 0) {
        retJson = $('.active').attr('jsonKey');
    }
    return retJson;
}

/**
 * 检查当前笔记是否包含录音条目
 * @return {boolean} 是否包含录音条目
 */
function hasVoice() {
    // 检查页面中是否存在语音块元素
    return $('.voiceBox').length > 0;
}



/**
 * 获取光标Y轴位置
 * @date 2021-09-08
 * @param {any} element 可编辑dom
 * @returns {number} Y轴位置 
 */
function getCursortPosition(element) {
    var caretOffset = 0;
    var doc = element.ownerDocument || element.document;
    var win = doc.defaultView || doc.parentWindow;
    var sel = win.getSelection();
    if (sel.rangeCount > 0) {
        var range = win.getSelection().getRangeAt(0);
        let span = document.createElement('span');
        $(span).addClass('focusAddress')
        range.insertNode(span)
        caretOffset = $(span).offset().top + ($(span).css('fontSize').slice(0, -2) - 0) + 5
        $(span).remove()
    }
    return caretOffset;
}

/**
 * 录音长度格式化
 * @date 2022-02-09
 * @param {any} millisecond  毫秒
 * @returns {any} "00:00"
 */
function formatMillisecond(millisecond) {
    if (0 == millisecond) {
        return '00:00'
    } else if (millisecond < 1000) {
        return '00:01'
    } else if (millisecond < 3600000) {
        var minutes = parseInt(millisecond / (1000 * 60));
        var seconds = parseInt((millisecond % (1000 * 60)) / 1000);
        return (minutes < 10 ? '0' + minutes : minutes) + ":" + (seconds < 10 ? '0' + seconds : seconds);
    }
    return "60:00";
}

//初始化数据 
function initData(text) {
    //打印text
    console.warn(text)

    initFinish = false;
    var arr = JSON.parse(text);
    var html = '';
    var voiceHtml;
    var txtHtml = '';
    console.warn(arr)
    arr.noteDatas.forEach((item, index) => {
        //false: txt
        if (item.type == 1) {
            txtHtml = ''
            if (item.text == '') {
                txtHtml = '<p><br></p>';
            }
            else {
                let textArr = item.text.split('\n')
                textArr.forEach(ite => {
                    txtHtml = txtHtml + '<p>' + ite + '</p>';
                })
            }
            html += txtHtml;
        }
        //true: voice
        else {
            if (!item.transSize) {
                item.transSize = formatMillisecond(item.voiceSize)
            }
            voiceHtml = transHtml(item, false);
            html += voiceHtml;
        }
    })

    $('#summernote').summernote('code', html);
    // 搜索功能
    webobj.jsCallSetDataFinsh();
    initFinish = true;
    $('#summernote').summernote('editor.resetRecord')
}

/**
 * 播放按钮置灰
 * @date 2021-09-03
 * @param {boolean} status false禁用 true启用
 * @returns {any}
 */
function playButColor(status) {
    // 更新录音状态（启用时不录音，禁用时在录音）
    global_isRecording = !status;
    console.log("playButColor: status=" + status + ", global_isRecording=" + global_isRecording);
    
    if (!status) {
        setVoiceButColor(global_disableColor, global_disableColor)

    } else {
        setVoiceButColor(global_activeColor, global_disableColor)
    }
}

//录音插入数据
function insertVoiceItem(text) {
    //插入语音之前先设置焦点
    $('#summernote').summernote('editor.focus')
    // 记录插入前数据
    $('#summernote').summernote('editor.recordUndo')

    var arr = JSON.parse(text);
    var voiceHtml = transHtml(arr, true);
    var oA = document.createElement('div');
    oA.className = 'li voiceBox';
    oA.contentEditable = false;
    oA.setAttribute('draggable', 'true');
    oA.setAttribute('jsonKey', text);
    oA.innerHTML = voiceHtml;

    var tmpNode = document.createElement("div");
    tmpNode.appendChild(oA.cloneNode(true));
    var str = '<p><br></p>' + tmpNode.innerHTML + '<p><br></p>';

    document.execCommand('insertHTML', false, str);
    // $('#summernote').summernote('editor.recordUndo')

    removeNullP()
    setFocusScroll()
    ensureVoiceBoxesDraggable();
}

/**
 * 移除无内容p标签
 * @date 2021-08-19
 * @returns {any}
 */
function removeNullP() {
    $('ul').children().each((index, item) => {
        if (item.tagName == 'P') {
            $(item).remove();
        }
    })
    $('ol').children().each((index, item) => {
        if (item.tagName == 'P') {
            $(item).remove();
        }
    })
    $('p').each((index, item) => {
        if (item.innerHTML === '') {
            $(item).remove();
        }
    })
}

/**
 * 更新语音播放控件状态
 * @param {jquery object} playback 语音播放控件 voicePlayback
 * @param {int} state 0,播放中，1暂停中，2.结束播放
 */
function updatePlayBackState(playback, state) {
    if (null === playback) {
        console.warn('No active voice playback object');
        return;
    }

    if (playback.length === 0 || !$.contains(document.documentElement, playback[0])) {
        return;
    }

    if (state == '0') {
        playback.removeClass('pause');
        playback.addClass('play');
    } else if (state == '1') {
        playback.addClass('pause');
    } else {
        // 重置状态
        var progressBar = playback.find('.progressBar').get(0);
        if (!progressBar) {
            return;
        }
        progressBar.value = 0;
        updateProgressBarValue(progressBar, progressBar.value);

        playback.removeClass('play').removeClass('pause');

        // 播放结束，关闭悬浮播放控件
        if (playback.is(airVoicePlayback)) {
            setAirVoicePlaybackVisible(false);
        }
    }
}

/**
 * 切换播放状态
 * @date 2021-08-19
 * @param {string} state 0,播放中，1暂停中，2.结束播放
 * @returns {any}
 */
function toggleState(state) {
    updatePlayBackState(activePlayback, state);
    updatePlayBackState(airVoicePlayback, state);
}

/**
 * 设置整个html内容
 * @date 2021-08-19
 * @param {string} html
 * @returns {any}
 */
function setHtml(html) {
    if (html == '<p></p>') {
        html = '<p><br></p>'
    }
    initFinish = false;
    $('#summernote').summernote('code', html);

    // TODO: 实验性功能，替换 HTML 文本中的语音控件版本，不涉及数据的变更
    replaceVoiceToVersion2();
    ensureVoiceBoxesDraggable();

    initFinish = true;
    // 搜索功能
    webobj.jsCallSetDataFinsh();
    resetScroll()
    $('#summernote').summernote('editor.resetRecord')

    // We need call once at init, ensure the foreground color / background color is correct.
    switchTextColor(global_theme == 2)
}

/**
 * 设置录音转文字内容
 * @param {string} text 转换文本内容，文本为空代表转失败了
 * @param {boolean} flag 0: 转换过程中 提示性文本（＂正在转文字中＂) １: 结果
 */
function setVoiceText(text, flag) {
    if (activeTransVoice) {
        if (flag) {
            // 需求/设计调整：不再直接追加转换文本到文件
            // if (text) {
            //     text = text.trim()
            //     activeTransVoice.after('<p>' + text + '</p>');
            //     webobj.jsCallTxtChange();
            // }

            if (text) {
                var translate = activeTransVoice.find('.translate');
                // 转Json数据绑定到模板
                var jsonData = {
                    translateLabel: divTranslateContent.translateLabel,
                    translateText: text
                }
                var textBindDataHandle = Handlebars.compile(translateTextContentTemplate);
                const translateTexthtml = textBindDataHandle(jsonData);
                translate.html(translateTexthtml);

                activeTransVoice.find('.voiceInfoBox').addClass('containText');
                translate.find('.translateHeader').addClass('unfold');
                translate.hide();
                translate.slideDown('fast');
            }

            //将转文字文本写到json属性里
            var jsonValue = activeTransVoice.attr('jsonKey');
            var jsonObj = JSON.parse(jsonValue);
            jsonObj.text = text;
            activeTransVoice.attr('jsonKey', JSON.stringify(jsonObj));

            // 复位 
            activeTransVoice.find('.voicePlayback').removeClass('voiceToText');
            webobj.jsCallTxtChange();
            activeTransVoice = null;
            bTransVoiceIsReady = true;

            // 移除选区
            var sel = window.getSelection();
            var range = sel.getRangeAt(0);
            sel.removeRange(range)
            $('.li').removeClass('active');
        }
        else {
            var voiceInfoBox = activeTransVoice.find('.voiceInfoBox');
            if (!voiceInfoBox.hasClass('containText')) {
                // 设置语音转换中标识，已有文本不再进行转换
                activeTransVoice.find('.voicePlayback').addClass('voiceToText');
                bTransVoiceIsReady = false;
            }
        }
    }
}

//json串拼接成对应html串 flag==》》 false: h5串  true：node串
function transHtml(json, flag) {
    // 设置翻译提示 div 文本内容
    json.translatingLabel = divTranslateContent.translatingLabel;

    let createTime = json.createTime.slice(0, 16)
    createTime = createTime.replace(/-/g, `/`)
    json.createTime = createTime
    //将json内容当其属性与标签绑定
    var strJson = JSON.stringify(json);
    json.jsonValue = strJson;
    var template;
    if (flag) {
        template = Handlebars.compile(nodeTplV2);
    } else {
        template = Handlebars.compile(h5TplV2);
    }
    var retHtml = template(json);
    return retHtml;
}

// 保证从历史/替换渲染回来的 voiceBox 都具有 draggable 属性
function ensureVoiceBoxesDraggable() {
    $('.voiceBox').attr('draggable', 'true');
}

//设置summerNote编辑状态 
function enableSummerNote() {
    if (activePlayback || (activeTransVoice && !bTransVoiceIsReady)) {
        $('#summernote').summernote('disable');
    }
    else {
        $('#summernote').summernote('enable');
    }
}

/**
 * 右键功能
 * @date 2021-08-19
 * @param {any} e
 * @returns {any} 
 */
function rightClick(e) {
    // isShowAir = false;
    var testDiv = getSelectedRange();
    let childrenLength = $(testDiv).children().length
    let voiceLength = $(testDiv).find('.voiceBox').length
    if (e.which == 3) {
        $('.li').removeClass('active');
    }
    if (e.target.tagName == 'IMG') {
        let img = e.target
        $('.note-editable ').blur()
        setSelectRange(img)
    } else if ($(e.target).hasClass('demo') || $(e.target).parents('.demo').length != 0) {
        $(e.target).parents('.li').addClass('active');
        // 当前没有语音在转文字时， 才可以转文字
        if (bTransVoiceIsReady) {
            activeTransVoice = $(e.target).parents('.li:first');
        }
        $('#summernote').summernote('airPopover.hide')
        setSelectRange($(e.target).parents('.voiceBox')[0])
    } else if ($(e.target).closest('.voicePlayback').length != 0) {
        // 适配新版语音控件(Version2)的右键：选中语音并弹出语音菜单
        $(e.target).closest('.li').addClass('active');
        if (bTransVoiceIsReady) {
            activeTransVoice = $(e.target).closest('.li:first');
        }
        $('#summernote').summernote('airPopover.hide');
        setSelectRange($(e.target).closest('.voiceBox')[0]);
    } else if (voiceLength == childrenLength && childrenLength != 0) {
        // selection中只有语音块，但右键点击位置不在语音块上，说明是旧的selection，需要清除
        if ($(e.target).closest('.voiceBox').length == 0) {
            // 点击的不是语音块，清除旧选区，让getSelectedRange()返回空内容
            window.getSelection().removeAllRanges();
        } else {
            // 点击的确实是语音块，记录它
            recordActiveVoice()
        }
    }
    let info = isRangeVoice()
    webobj.jsCallPopupMenu(info.flag, info.info);
    // 阻止默认右键事件
    // e.preventDefault()
}


/**
 * 设置活动色
 * @date 2021-09-06
 * @param {any} color
 * @returns {any}
 */
function setVoiceButColor(color, shdow) {
    // 判断是否为禁用状态（颜色和阴影都是禁用色）
    var isDisabled = (color === global_disableColor && shdow === global_disableColor);
    
    $("#style").html(`
    :root {
        --highlightColor: ${color};
    }

    .voiceBox .voiceBtn {
        background-color: ${color};
        box-shadow: 0px 4px 6px 0px ${shdow}80;
        ${isDisabled ? 'filter: grayscale(100%) opacity(0.5);' : ''}
    } 
    ::selection {
        background: ${color}!important;
    }
    .btn-default.active i {
        color:${color}!important
    }
    .btn-default:active {
        color:${color}!important
    }
    .btn-default i:active {
        color:${color}!important
    }
    .selectColor {
        box-shadow: 0 0 0 1.5px ${color};
    }
    .active {
        background: ${color}80!important;
    }

    .li .translateText::selection {
        background: ${color}40 !important;
        color: inherit;
    }
    
    .li.active .translateText::selection {
        background: ${color}60 !important;
        color: inherit;
    }    
    `)
}

/**
 * 深色浅色变换
 * @date 2021-08-19
 * @param {any} flag 1浅色 2深色
 * @param {any} activeColor 选中颜色
 * @param {any} disableColor 禁用颜色
 * @param {any} backgroundColor 现在背景色主要由 qml 组件和 web 前端 css 共同实现，但在 sw 下保留兼容设置
 * @returns {any}
 */
function changeColor(flag, activeColor, disableColor, backgroundColor) {
    global_theme = flag
    global_activeColor = activeColor
    global_disableColor = disableColor
    global_themeColor = backgroundColor
    // 根据录音状态设置按钮颜色：录音时置灰，非录音时正常
    if (global_isRecording) {
        setVoiceButColor(global_disableColor, global_disableColor)
    } else {
        setVoiceButColor(global_activeColor, global_disableColor)
    }

    $('.dropdown-fontsize>li>a').hover(function (e) {
        $(this).css('background-color', activeColor);
    }, function () {
        $('.dropdown-fontsize>li>a').css('background-color', 'transparent');
        if (flag == 1) {
            $('.dropdown-fontsize>li>a').css('color', "black");
        } else {
            $('.dropdown-fontsize>li>a').css('color', "rgba(197,207,224,1)");
        }
    })
    $('.dropdown-fontname>li>a').hover(function (e) {
        $(this).css('background-color', activeColor);
    }, function () {
        $('.dropdown-fontname>li>a').css('background-color', 'transparent');
        if (flag == 1) {
            $('.dropdown-fontname>li>a').css('color', "black");
        } else {
            $('.dropdown-fontname>li>a').css('color', "rgba(197,207,224,1)");
        }
    })

    if (flag == 1) {
        $('body').removeClass('dark');
        $('#dark').remove()
        $('.dropdown-fontsize>li>a').css('color', "black");
        $('.dropdown-fontname>li>a').css('color', "black");
    } else if (flag == 2 && !$('#dark').length) {
        $('body').addClass('dark');
        $("head").append("<link>");
        var css = $("head").children(":last");
        css.attr({
            id: 'dark',
            rel: "stylesheet",
            type: "text/css",
            href: "./css/dark.css"
        });
    }

    // 设置下拉菜单调色板
    $('#summernote').summernote('airPopover.updateColorPalette', global_theme == 2);
    // change text foreground color and background color with theme
    switchTextColor(global_theme == 2);
}

/**
 * 录音块拖拽（语义级交换）：
 * - 仅允许在 .voiceBox 与 .voiceBox 之间拖拽
 * - 阻止浏览器默认的 HTML 片段拖拽复制行为
 * - 交换双方的 jsonKey，并用模板重渲染各自的内部结构
 * - 保留容器节点与属性（包括 draggable/contenteditable 等），避免事件与状态丢失
 */
function initVoiceDragAndDrop() {
    var dragSrc = null;

    // 监听编辑区内任意节点的 dragstart，归一到最近的 voiceBox 作为拖拽源
    $('body').on('dragstart', '.note-editable *', function (e) {
        var vb = $(e.target).closest('.voiceBox').get(0);
        if (!vb) return;
        dragSrc = vb;
        var dt = e.originalEvent.dataTransfer;
        if (dt) {
            dt.effectAllowed = 'move';
            dt.setData('text/plain', 'VOICEBOX');
        }
    });

    // 允许在编辑区内的任意子节点上方放置，使用 closest 命中 voiceBox
    // 允许在编辑区任意位置拖拽经过，不强制要求命中 voiceBox
    $('body').on('dragenter dragover', '.note-editable, .voiceBox', function (e) {
        if (!dragSrc) return;
        e.preventDefault();
        var dt = e.originalEvent.dataTransfer;
        if (dt) dt.dropEffect = 'move';
    });


    $('body').on('drop', '.note-editable, .voiceBox', function (e) {
        if (!dragSrc) return;
        // 首先尝试命中目标块
        var dstBox = $(e.target).closest('.voiceBox').get(0);
        // 若未命中目标块，则按鼠标 Y 坐标选择最近的 voiceBox
        if (!dstBox) {
            var clientY = (e.originalEvent && (e.originalEvent.clientY || (e.originalEvent.touches && e.originalEvent.touches[0] && e.originalEvent.touches[0].clientY))) || 0;
            var nearest = null;
            var minDist = Infinity;
            $('.note-editable .voiceBox').each(function () {
                var rect = this.getBoundingClientRect();
                var mid = (rect.top + rect.bottom) / 2;
                var dist = Math.abs(clientY - mid);
                if (dist < minDist) {
                    minDist = dist;
                    nearest = this;
                }
            });
            dstBox = nearest;
        }
        if (!dstBox) { dragSrc = null; return; }
        e.preventDefault();

        if (dragSrc === dstBox) { dragSrc = null; return; }

        // 只交换 jsonKey 与内部渲染，保留容器节点
        var srcJson = $(dragSrc).attr('jsonKey');
        var dstJson = $(dstBox).attr('jsonKey');

        function renderVoiceBox(container, jsonStr) {
            var json = JSON.parse(jsonStr);
            var html = transHtml(json, true);
            $(container).attr('jsonKey', jsonStr);
            // 覆盖容器内部（voiceInfoBox + translate），保持容器属性不变
            $(container).html(html);
        }

        renderVoiceBox(dragSrc, dstJson);
        renderVoiceBox(dstBox, srcJson);

        // 状态复位
        $(dragSrc).find('.voicePlayback').removeClass('now play pause voiceToText');
        $(dstBox).find('.voicePlayback').removeClass('now play pause voiceToText');
        activePlayback = null;
        enableSummerNote();
        ensureVoiceBoxesDraggable();

        dragSrc = null;
    });

    $('body').on('dragend', '.voiceBox', function () {
        dragSrc = null;
    });
}

/**
 * 插入图片
 * @date 2021-08-19
 * @param {any} urlStr 图片地址list
 * @returns {any}
 */
async function insertImg(urlStr) {
    urlStr.forEach((item, index) => {
        $("#summernote").summernote('insertImage', item, 'img');
    })
}

/**
 * trigger play/pause voice when key space / return / enter down
 */
function triggerPlayPauseVoice(event) {
    var currentLi = $('.li.active');
    if (currentLi.length > 0) {
        event.preventDefault();
        var info = isRangeVoice();
        if (info.flag == 1) {
            var curPlayback = currentLi.first().find('.voicePlayback');
            // 点击浮动窗口时视同点击焦点音频播放控件
            if (curPlayback.is(airVoicePlayback)) {
                curPlayback = activePlayback;
            }
            var jsonString = currentLi.attr('jsonKey');
            var bIsSame = curPlayback.hasClass('now');

            if (!bIsSame) {
                if (null !== activePlayback && activePlayback.hasClass('now')) {
                    // 移除之前控件的状态，重置样式
                    activePlayback.removeClass('now').removeClass('play').removeClass('pause');
                }
                curPlayback.addClass('now');
                activePlayback = curPlayback;

                // 重置悬浮工具栏状态
                resetAirVoicePlayback();
            }

            // 后端播放处理，等待后端开始播放音频数据 -> callJsSetPlayStatus()
            webobj.jsCallPlayVoice(jsonString, bIsSame, function (state) {
                //TODO 录音错误处理
            });

            return true;
        }
    }

    return false;
}

/**
 * Capture keydown event on capture phase
 */
document.addEventListener('keydown', function (event) {
    if (event.key === 'Enter' || event.keyCode === 13) {
        triggerPlayPauseVoice(event)
    }
}, true); // Key: The third parameter is set to true, indicating the capture phase

/**
 * Capture keydown event on propagation phase
 */
document.onkeydown = function (event) {
    // 检查是否按下Ctrl+D组合键，如果是则阻止默认行为并直接触发保存录音
    if (event.ctrlKey && window.event.keyCode == 68) {
        // ctrl+d
        event.preventDefault();
        // 只有当前笔记中有录音条目时才触发保存操作
        if (hasVoice()) {
            webobj.jsCallSaveAudio();
        } else {
            console.log("当前笔记中没有录音条目，不执行保存操作");
        }
        return false;
    }
    
    if (window.event.keyCode == 13) {
        // key enter or return
        setFocusScroll()

    } else if (event.ctrlKey && window.event.keyCode == 86) {
        // ctrl+v
        webobj.jsCallPaste(returnCopyFlag())
        return false;

    } else if (window.event.keyCode == 32) {
        // space
        triggerPlayPauseVoice(event);

    } else if (window.event.keyCode == 8) {
        // backspace
        // 检查是否选中了语音块
        var testDiv = getSelectedRange();
        if ($(testDiv).find('.voiceBox').length == $(testDiv).children().length && $(testDiv).children().length != 0) {
            if (activePlayback && activePlayback.hasClass('play')) {
                console.log("Cannot delete voice while playing");
                return false;
            }
            deleteSelection();
            return false;
        }
        
        if (getSelectedRange().innerHTML == document.querySelector('.note-editable').innerHTML) {
            $('.note-editable').html('<p><br></p>')
        }

        // 移除行内样式的p元素
        let styleStr = $('p[style="display: inline !important;"]')
        styleStr.after(styleStr.html()).remove();

        var selectionObj = window.getSelection();
        let focusDom = selectionObj.extentNode
        // 判断是选区还是光标
        if (selectionObj.type != 'Caret') {
            return
        }
        
        // 检查光标前面是否是语音块（从普通文本向语音块删除）
        if (selectionObj.anchorOffset === 0 && focusDom.nodeType === 3) {
            var $parent = $(focusDom).parent();
            var $prevElement = $parent.prev();
            
            // 如果前一个元素是语音块
            if ($prevElement.hasClass('voiceBox')) {
                if (activePlayback && activePlayback.hasClass('play')) {
                    console.log("Cannot delete voice while playing");
                    return false;
                }
                setSelectRange($prevElement[0]);
                deleteSelection();
                return false;
            }
            
            // 如果父元素的前一个元素是语音块
            if ($parent.is('p') && $parent.prev().hasClass('voiceBox')) {
                if (activePlayback && activePlayback.hasClass('play')) {
                    console.log("Cannot delete voice while playing");
                    return false;
                }
                setSelectRange($parent.prev()[0]);
                deleteSelection();
                return false;
            }
        }
        if ($(focusDom).closest('span').length
            && $(focusDom).closest('span').html() !== '<br>'
            && selectionObj.extentOffset == 0) {
            if ($(focusDom).parents('li').length
                && $(focusDom).parents('li').prev().length
                && $(focusDom).parents('li').prev()[0].tagName == 'LI'
                && !$(focusDom).parents('li').find('.voiceBox').length) {
                // 父元素为LI
                setBackspace(focusDom, "li")
                return false
            }
            else if ($(focusDom).parents('p').length
                && $(focusDom).parents('p').prev().length
                && $(focusDom).parents('p').prev()[0].tagName == 'UL') {
                // 父元素为P 上个元素为UL
                setBackspace(focusDom, "p")
                return false
            }
            else if ($(focusDom).parents('p').length
                && $(focusDom).parents('p').prev().length
                && $(focusDom).parents('p').prev()[0].tagName == 'OL') {
                // 父元素为P 上个元素为OL
                setBackspace(focusDom, "p")
                return false
            }
        }
    } else if (window.event.keyCode == 46) {
        // delete
        // 检查是否选中了语音块
        var testDiv = getSelectedRange();
        if ($(testDiv).find('.voiceBox').length == $(testDiv).children().length && $(testDiv).children().length != 0) {
            // 选中的是语音块，检查是否有语音正在播放
            if (activePlayback && activePlayback.hasClass('play')) {
                console.log("Cannot delete voice while playing");
                return false;
            }
            // 没有播放，允许删除
            deleteSelection();
            return false;
        }
        
        var selectionObj = window.getSelection();
        let focusDom = selectionObj.extentNode
        let domList = $(focusDom).parents()
        let isHas = true
        // 判断是选区还是光标
        if (selectionObj.type != 'Caret') {
            return
        }
        // 处理LI以内子元素
        for (let i = 0; i < domList.length; i++) {
            if (domList[i].tagName == 'LI') {
                let nowItem = domList[i]
                domList = domList.slice(0, i);
                if (domList.length == 0) {
                    domList.push(nowItem.firstChild)
                }
                break;
            }
        }
        domList = domList.toArray()
        // 判断是否有除br的以外的兄弟元素
        for (let i = 0; i < domList.length; i++) {
            if (domList[i].nextSibling) {
                if (domList[i].nextSibling.nodeType == 3 || (domList[i].nextSibling.nodeType != 3 && !domList[i].nextSibling.innerHTML.match(/<br>/g))) {
                    isHas = false;
                    break;
                } else if (domList[i].nextSibling.innerHTML.match(/<br>/g)) {
                    $(domList[i].nextSibling).remove()
                    isHas = true;
                }
            } else {
                isHas = true;
            }
        }

        if (selectionObj.extentOffset == focusDom.nodeValue.length && isHas) {
            if ($(focusDom).parents('li').length
                && $(focusDom).parents('li').next().length
                && $(focusDom).parents('li').next()[0].tagName == 'LI'
                && !$(focusDom).parents('li').find('.voiceBox').length
                && $(focusDom).parents('li').next().children('span').length) {
                // 下个元素为LI
                setDelete(focusDom, "li")
                return false
            }
            else if (!$(focusDom).parents('li').next().length
                && $(focusDom).parents('ul').next().length
                && $(focusDom).parents('ul').next()[0].tagName == 'P'
                && $(focusDom).parents('ul').next().children('span').length) {
                // 下个元素为P
                setDelete(focusDom, "ul")
                return false
            }
            else if (!$(focusDom).parents('li').next().length
                && $(focusDom).parents('ol').next().length
                && $(focusDom).parents('ol').next()[0].tagName == 'P'
                && $(focusDom).parents('ol').next().children('span').length) {
                // 下个元素为P 
                setDelete(focusDom, "ol")
                return false
            }
        }
    } else if (event.ctrlKey && window.event.keyCode == 65) {
        // ctrl+a
        // 去除语音选中样式
        $('.li').removeClass('active');
    } else if (window.event.keyCode == 46) {
        // delete
        if (getSelectedRange().innerHTML == document.querySelector('.note-editable').innerHTML) {
            $('.note-editable').html('<p><br></p>')
        }
    }

}

/**
 * delete 改变元素位置
 * @date 2021-11-25
 * @param {dom} focusDom 焦点所在元素
 * @param {string} name 父元素名
 * @returns {any}
 */
function setDelete(focusDom, name) {
    let nextDom = $(focusDom).closest(name).next()
    if (name == 'li') {
        $(focusDom).parents(name).append($(focusDom).parents('li').next())
    } else {
        $(focusDom).closest('li').append(nextDom)
    }
    nextDom.children(":first").unwrap()
}

/**
 * backspace 改变元素位置
 * @date 2021-11-25
 * @param {dom} focusDom 焦点所在元素
 * @param {string} name 父元素名
 * @returns {any}
 */
function setBackspace(focusDom, name) {
    if (name == 'li') {
        $(focusDom).parents(name).prev().append($(focusDom).parents('li'))
    } else {
        $(focusDom).parents(name).prev().children(':last').append($(focusDom).parents(name))
    }
    let prevDom = $(focusDom).closest(name).prev()
    if (prevDom.length && prevDom[0].tagName == "SPAN" && prevDom.html().match(/<br>/g)) {
        prevDom.remove()
    }
    $(focusDom).parents(name).children(":first").unwrap()
    setFocus($(focusDom).parent()[0], 0)
}

// ctrl+b 添加记事本
// ctrl+d 保存录音
document.onkeyup = function (event) {
    if (event.ctrlKey && window.event.keyCode == 66) {
        webobj.jsCallCreateNote()
    }
    // 注释掉这里的Ctrl+D处理，因为我们已经在keydown事件中处理了
    // else if (event.ctrlKey && window.event.keyCode == 68) {
    //     webobj.jsCallSaveAudio()
    // }
}

// 图片双击预览
$('body').on('dblclick', 'img', function (e) {
    e.stopPropagation()
    e.preventDefault()
    let imgUrl = $(e.target).attr('src')
    webobj.jsCallViewPicture(imgUrl)
})

// 图片单击选中
$('body').on('click', 'img', function (e) {
    let img = e.target
    setSelectRange(img)
})

/**
 * 右键显示悬浮工具栏
 * @date 2021-09-03
 * @param {any} x 坐标
 * @param {any} y 坐标
 * @returns {any}
 */
function showRightMenu(x, y) {
    $('#summernote').summernote('airPopover.rightUpdate', x, y)
}

// 右键隐藏悬浮工具栏
function hideRightMenu() {
    $('#summernote').summernote('airPopover.hide')
}

// 重置滚动条
function resetScroll() {
    if ($(document).scrollTop() != 0) {
        $(document).scrollTop(0)
    }
}

// 监听滚动事件
$(document).scroll(function () {
    var scrollTopDistance = $(document).scrollTop();
    webobj.jsCallScrollChange(scrollTopDistance);

    if (scrollHide) {
        clearTimeout(scrollHide)
        $('#scrollStyle').html(`
        body::-webkit-scrollbar-thumb {
        background-color:${global_theme == 1 ? "rgba(0, 0, 0, 0.30)" : "rgba(255, 255, 255, 0.20)"} ;
        }
        /* 适配申威，触发重绘 */
        html {
            background-color: ${global_themeColor}fe;
        }
        `
        )
    }

    scrollHide = setTimeout(() => {
        $('#scrollStyle').html('')
    }, 1500);
});

// 字体滚动条
function listenFontnameList() {
    $('.dropdown-fontname ').scroll(function () {
        // 阻止内部滚动条到底后自动触发外部滚动
        const scroll = document.querySelector('.dropdown-fontname')
        // 当前滚动条距离底部还剩2px时
        if (scroll.scrollHeight - (scroll.scrollTop + scroll.clientHeight) <= 2) {
            // 定位到距离底部2px的位置
            scroll.scrollTop = (scroll.scrollHeight - scroll.clientHeight - 2)
        }

        if (scrollHideFont) {
            clearTimeout(scrollHideFont)
            $('#scrollStyleFont').html(`
        .dropdown-fontname::-webkit-scrollbar-thumb {
        background-color:${global_theme == 1 ? "rgba(0, 0, 0, 0.30)" : "rgba(255, 255, 255, 0.20)"} ;
        }
        /* 适配申威，触发重绘 */
        html {
            background-color: ${global_themeColor}fe;
        }
        `
            )
        }

        scrollHideFont = setTimeout(() => {
            $('#scrollStyleFont').html('')
        }, 1500);
    });
}


/**
 * 改变光标颜色
 * @date 2021-09-09
 * @param {int} flag 1 透明 2 自动
 * @returns {any}
 */
function setFocusColor(flag) {
    $('.note-editable').css('caret-color', flag == 1 ? 'transparent' : 'auto')
}

// 复制标志
function returnCopyFlag() {
    return isVoicePaste
}

// 当前选中颜色active
function setSelectColorButton($dom) {
    $dom.parents('.note-color-palette').find('.note-color-btn').removeClass('selectColor')
    $dom.addClass('selectColor')
}

/**
 * 删除当前选中文本
 */
function deleteSelection() {
    $('#summernote').summernote('editor.deleteContents');
}

/***** 以下为语音播放相关函数 *****/

/**
 * 替换语音控件为 Version2 版本，数据不会变更，继续使用原有 html 记录的 json 数据。
 */
function replaceVoiceToVersion2() {
    $('.voiceInfoBox:not(.version2)').each(function (index, item) {
        var jsonKeyString = $(item).parents('.voiceBox:first').attr('jsonKey');
        if (!jsonKeyString) {
            return;
        }

        var originJsonData = JSON.parse(jsonKeyString);
        originJsonData.translatingLabel = divTranslateContent.translatingLabel;

        var htmlBindDataHandle = Handlebars.compile(nodeTplV2);
        const newVoiceInfoBoxHtml = htmlBindDataHandle(originJsonData);
        $(item).replaceWith(newVoiceInfoBoxHtml);
    });
}

/**
 * 切换悬浮语音播放控件显示状态
 * @param {boolean} visible 是否显示
 */
function setAirVoicePlaybackVisible(visible) {
    if (visible) {
        airVoicePlayback.parents('.airVoicePlaybackContainer').fadeIn(366);
    } else {
        airVoicePlayback.parents('.airVoicePlaybackContainer').fadeOut(366);
    }
}

/*
 * 点击悬浮语音播放控件关闭按钮，停止播放，重置语音状态
 */
$('body').on('click', '.closePlaybackBarBtn', function (e) {
    webobj.jsCallPlayVoiceStop();
})

/**
 * 检测当前语音播放控件是否在可视范围内，若在可视范围内，则更新浮动窗口状态
 */
function checkVoicePlaybackViewport() {
    if (null === activePlayback || !activePlayback.hasClass('play')) {
        return;
    }

    // 获取原生 DOM 对象
    var rect = activePlayback.get(0).getBoundingClientRect();

    var inViewport = Boolean(
        rect.top >= 0 &&
        rect.left >= 0 &&
        rect.bottom <= (window.innerHeight || document.documentElement.clientHeight) &&
        rect.right <= (window.innerWidth || document.documentElement.clientWidth)
    );

    setAirVoicePlaybackVisible(!inViewport);
}

window.addEventListener('scroll', checkVoicePlaybackViewport)
window.addEventListener('resize', checkVoicePlaybackViewport)

function resetAirVoicePlayback() {
    if (null === activePlayback) {
        return;
    }

    // 重置时当前voicePlayback一定在可视范围内
    setAirVoicePlaybackVisible(false);

    // 同步数据
    airVoicePlayback.find('.title').text(activePlayback.find('.title').text());
    airVoicePlayback.find('.timeTotal').text(activePlayback.find('.timeTotal').text());
    airVoicePlayback.find('.progressBar').get(0).max = parseFloat(activePlayback.find('.progressBar').get(0).max);
}

/**
 * 添加悬浮的语音播放控件
 */
function addAirVoicePlayback() {
    var airEle = document.createElement('div');
    airEle.className = 'airVoicePlaybackContainer';
    airEle.innerHTML = airVoicePlayBackTemplate;
    $('#summernote').after(airEle);

    airVoicePlayback = $(airEle).find('.voicePlayback');
    $(airEle).hide();
}

/**
 * 当前进度条数值被手动拖拽，释放时触发
 */
function onProgressChange() {
    const target = event.target
    updateProgressBarValue(target, target.value);
    progressBarDraging = false;

    // 手动触发进度变更
    webobj.jsCallVoiceProgressChange(target.value);
}

/**
 * 当前进度条输入变更时触发
 */
function onProgressInput() {
    progressBarDraging = true;
    updateProgressBarValue(event.target, event.target.value);
}

/**
 * 后端通知前端，语音播放进度变更时触发，更新进度条样式
 * @param {int} value 当前已播放进度值 ms
 */
function updateProgressBar(value) {
    if (progressBarDraging) {
        return;
    }

    if (null !== activePlayback && activePlayback.hasClass('now')) {
        if (!$.contains(document.documentElement, activePlayback[0])) {
            activePlayback = null;
            return;
        }
        var progressBar = activePlayback.find('.progressBar').get(0);
        if (!progressBar) {
            return;
        }
        // 按秒进位
        progressBar.value = (value / 1000) * 1000;
        updateProgressBarValue(progressBar, progressBar.value);

        // 同步更新浮动窗口进度条
        var airProgressBar = airVoicePlayback.find('.progressBar').get(0);
        if (airProgressBar) {
            airProgressBar.value = progressBar.value;
            updateProgressBarValue(airProgressBar, progressBar.value);
        }
    }
}

/**
 * 语音播放进度tick变更时触发，更新进度条样式
 * @param {dom} target 进度条对象
 * @param {int} value 当前进度tick值 ms
 */
function updateProgressBarValue(target, value) {
    if (0 == target.max) {
        console.error('updateProgressBar error: max is 0');
        return;
    }

    const ratio = (parseFloat(value) / parseFloat(target.max)) * 100;
    // 设置自定义属性以控制伪元素的进度条宽度
    target.style.setProperty('--progressValue', `${ratio}%`);

    // 更新实时进度时间
    var timePassed = formatMillisecond(value) + '/';
    $(activePlayback).find('.timePassed').text(timePassed);
    $(airVoicePlayback).find('.timePassed').text(timePassed);
}

/***** 在语音播放控件内区分拖拽语义：进度条区域拖动 -> 调整进度；其他区域 -> 保持原有行为 *****/
function getClientPos(e) {
    var oe = e.originalEvent || e;
    if (oe.touches && oe.touches.length) {
        return { x: oe.touches[0].clientX, y: oe.touches[0].clientY };
    }
    return { x: oe.clientX, y: oe.clientY };
}

function seekProgressByClientX(progressBar, clientX) {
    var rect = progressBar.getBoundingClientRect();
    var ratio = (clientX - rect.left) / rect.width;
    if (isNaN(ratio)) {
        return;
    }
    ratio = Math.max(0, Math.min(1, ratio));
    var max = parseFloat(progressBar.max || 0);
    if (max <= 0) {
        return;
    }
    var value = ratio * max;
    // 以秒为步进，和原有 step=1000 保持一致
    progressBar.value = Math.round(value / 1000) * 1000;
    updateProgressBarValue(progressBar, progressBar.value);
}

// 在 voicePlayback 内监听起始事件；若点击在 progressBar 可视区域内，则进入进度拖拽模式
$('body').on('mousedown pointerdown touchstart', '.voicePlayback', function (e) {
    var progressBar = $(this).find('.progressBar').get(0);
    if (!progressBar) {
        return;
    }
    var pos = getClientPos(e);
    var rect = progressBar.getBoundingClientRect();
    var inside = (pos.x >= rect.left && pos.x <= rect.right && pos.y >= rect.top && pos.y <= rect.bottom);
    if (!inside) {
        // 非进度条区域：保持原有选中/拖拽语义
        return;
    }

    // 进度条区域：改为进度拖拽语义
    e.preventDefault();
    e.stopPropagation();

    progressBarDraging = true;
    seekProgressByClientX(progressBar, pos.x);

    var move = function (ev) {
        var p = getClientPos(ev);
        seekProgressByClientX(progressBar, p.x);
        if (ev.preventDefault) ev.preventDefault();
    };
    var up = function (ev) {
        var p2 = getClientPos(ev);
        seekProgressByClientX(progressBar, p2.x);
        progressBarDraging = false;
        if (typeof webobj !== 'undefined' && webobj && webobj.jsCallVoiceProgressChange) {
            webobj.jsCallVoiceProgressChange(progressBar.value);
        }
        $(window).off('mousemove pointermove touchmove', move);
        $(window).off('mouseup pointerup touchend', up);
        if (ev.preventDefault) ev.preventDefault();
    };

    $(window).on('mousemove pointermove touchmove', move);
    $(window).on('mouseup pointerup touchend', up);
});

/***** 语音播放相关函数 end *****/

/***** 以下为语音转文本相关函数 *****/

/**
 * 点击语音转文本标题，切换语音转文本展开/折叠状态
 */
$('body').on('click', '.translateHeader', function (e) {
    var target = $(e.target);
    if (target.hasClass('translateHeader')) {
        toggleVoiceToTextState(target);
    } else {
        toggleVoiceToTextState(target.parents('.translateHeader:first'));
    }
})

function toggleVoiceToTextState(translateHeader) {
    if (!translateHeader) {
        return;
    }

    if (translateHeader.hasClass('unfold')) {
        translateHeader.removeClass('unfold');
        translateHeader.next('.translateText').slideUp('fast');
    } else {
        translateHeader.addClass('unfold');
        translateHeader.next('.translateText').slideDown('fast');
    }
}

/***** 语音转文本相关函数 end *****/

function exchangeTextColor(oldForeColors, newForeColors, oldBackColors, newBackColors) {
    document.querySelectorAll('.note-editable span').forEach(function (item) {
        if (item.style.color) {
            var index = oldForeColors.indexOf(item.style.color);
            if (index >= 0 && index < newForeColors.length) {
                item.style.color = newForeColors[index];
            }
        }

        if (item.style.backgroundColor) {
            var backIndex = oldBackColors.indexOf(item.style.backgroundColor);
            if (backIndex >= 0 && backIndex < newBackColors.length) {
                item.style.backgroundColor = newBackColors[backIndex];
            }
        }
    });
}

/**
 * @brief Change text background/foreground color when switching themes
 */
function switchTextColor(isDarkTheme) {
    var options = $('#summernote').data('summernote').options;

    if (isDarkTheme) {
        exchangeTextColor(options.simpleLightForeColors.flat(), options.simpleDarkForeColors.flat(),
            options.simpleLightBackColors.flat(), options.simpleDarkBackColors.flat());
    } else {
        exchangeTextColor(options.simpleDarkForeColors.flat(), options.simpleLightForeColors.flat(),
            options.simpleDarkBackColors.flat(), options.simpleLightBackColors.flat());
    }
}

/**
 * @brief When you click the drop-down menu, adjust the display position
 * according to the available height of the window.
 */
$('body').on('click', '.dropdown-toggle', function (e) {
    const clickedItem = this;
    const parentItem = clickedItem.parentElement;
    const needClose = parentItem.classList.contains('open');

    const menuItem = clickedItem.nextElementSibling;
    if (menuItem && menuItem.classList.contains('dropdown-menu')) {
        if (needClose) {
            // reset css config
            menuItem.style.top = '115%';
            return;
        }
        // temporarily set display to block to get the height of the menu
        menuItem.classList.add('show');

        var dropdownRect = clickedItem.getBoundingClientRect();
        var menuRect = menuItem.getBoundingClientRect();
        var belowSpace = window.innerHeight - dropdownRect.bottom;
        // the margin with dropdown button and menu
        var offset = 1.35 * dropdownRect.height;
        // top below great than bottom below, bottom below cannot show whole menu
        var showTop = (dropdownRect.top > belowSpace);
        if (belowSpace > (menuRect.height + offset)) {
            showTop = false;
        }

        // reset display to none
        menuItem.classList.remove('show');

        if (showTop) {
            menuItem.style.top = '-' + (menuRect.height + 10) + 'px';
        } else {
            menuItem.style.top = '115%';
        }
    }
})
