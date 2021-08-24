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

var h5Tpl = `
    <div class="li voiceBox" contenteditable="false" jsonKey="{{jsonValue}}">
        <div class='voiceInfoBox'>
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
                {{#if text}}
                <div>{{text}}</div>
                {{/if}}
            </div>
        </div>
    </div>`;

var nodeTpl = `
        <div class='voiceInfoBox'>
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
                {{#if text}}
                <div>{{text}}</div>
                {{/if}}
            </div>
        </div>`;

var formatHtml = ''
var pasteData = "";
var webobj;    //js与qt通信对象
var activeVoice = null;  //当前正操作的语音对象
var activeTransVoice = null;  //执行语音转文字对象
var bTransVoiceIsReady = true;  //语音转文字是否准备好
var initFinish = false;
var voiceIntervalObj;    //语音播放动画定时器对象

// 初始化summernote
$('#summernote').summernote({
    focus: true,
    disableDragAndDrop: true,
    shortcuts: false,
    lang: 'zh-CN',
    popover: {
        air: [
            ['fontsize', ['fontsize']],
            ['forecolor', ['forecolor']],
            ['backcolor', ['backcolor']],
            ['bold', ['bold']],
            ['italic', ['italic']],
            ['underline', ['underline']],
            ['strikethrough', ['strikethrough']],
            ['ul', ['ul']],
            ['ol', ['ol']],
        ],
    },
    airMode: true,
});

// 监听窗口大小变化
$(window).resize(function () {
    $('.note-editable').css('min-height', $(window).height())
});

/**
 * 通知后台存储页面内容
 * @date 2021-08-19
 * @param {any} we
 * @param {any} contents
 * @param {any} $editable
 * @returns {any}
 */
function changeContent(we, contents, $editable) {
    if (webobj && initFinish) {
        webobj.jsCallTxtChange();
    }
}

// 判断编辑区是否为空
function isNoteNull() {
    return $('.note-editable').html() === '<p><br></p>'
}

//点击选中录音
$('body').on('click', '.li', function (e) {
    console.log('div click...');
    e.stopPropagation();
    $('.li').removeClass('active');
    // $(this).addClass('active');
})

// 取消选中
$('body').on('click', function () {
    $('.li').removeClass('active');
})

// 语音复制粘贴
document.addEventListener('copy', function (event) {
    var selectionObj = window.getSelection();
    var rangeObj = selectionObj.getRangeAt(0);
    var docFragment = rangeObj.cloneContents();
    var testDiv = document.createElement("div");
    testDiv.appendChild(docFragment);
    formatHtml = testDiv.innerHTML;
    pasteData = window.getSelection().toString();
    // if (formatHtml.substr(0, 11) != "<p><br></p>") {
    //     formatHtml = "";
    // }
});

document.addEventListener('paste', function (event) {
    if (formatHtml != "" && pasteData == event.clipboardData.getData('Text')) {
        document.execCommand('insertHTML', false, formatHtml + "<p><br></p>");
    }
    removeNullP()
});

//播放
$('body').on('click', '.voicebtn', function (e) {
    console.log('------playBtn click...');
    // e.stopPropagation();
    var curVoice = $(this).parents('.li:first');
    var jsonString = curVoice.attr('jsonKey');
    var bIsSame = $(this).hasClass('now');
    var curBtn = $(this);
    $('.voicebtn').removeClass('now');
    activeVoice = curBtn;
    activeVoice.addClass('now');

    webobj.jsCallPlayVoice(jsonString, bIsSame, function (state) {
        //TODO 录音错误处理
    });
})

//获取整个处理后Html串,去除所有标签中临时状态
function getHtml() {
    // var rightCode = $('#summernote').summernote('code');
    // $('.li').removeClass('active');

    // if (activeVoice)
    // {
    //     activeVoice.removeClass('pause').addClass('play');
    //     activeVoice.addClass('now');
    // }

    // var handleCode = $('#summernote').summernote('code');
    // setHtml(rightCode);
    // return handleCode;

    return $('#summernote').summernote('code');
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

new QWebChannel(qt.webChannelTransport,
    function (channel) {
        console.log('qt.webChannelTransport....');
        webobj = channel.objects.webobj;
        //所有的c++ 调用js的接口都需要在此绑定格式，webobj.c++函数名（jscontent.cpp查看.connect(js处理函数)
        //例如 webobj.c++fun.connect(jsfun)
        webobj.callJsInitData.connect(initData);
        webobj.callJsInsertVoice.connect(insertVoiceItem);
        webobj.callJsSetPlayStatus.connect(toggleState);
        webobj.callJsSetHtml.connect(setHtml);
        webobj.callJsSetVoiceText.connect(setVoiceText);
        webobj.callJsInsertImages.connect(insertImg);
        //通知QT层完成通信绑定
        webobj.jsCallChannleFinish();
    }
)

//初始化数据 
function initData(text) {
    initFinish = false;
    console.log('=============>initData', text);
    var arr = JSON.parse(text);
    var html = '';
    var voiceHtml;
    var txtHtml;

    arr.noteDatas.forEach((item, index) => {
        //false: txt
        if (item.type == 1) {
            console.log('---txt---');
            if (item.text == '') {
                txtHtml = '<p><br></p>';
            }
            else {
                txtHtml = '<p>' + item.text + '</p>';
            }
            html += txtHtml;
        }
        //true: voice
        else {
            console.log('---voice---');
            voiceHtml = transHtml(item, false);
            html += voiceHtml;
        }
    })

    $('#summernote').summernote('code', html);
    // 搜索功能
    webobj.jsCallSetDataFinsh();
    initFinish = true;
}

//录音插入数据
function insertVoiceItem(text) {
    console.log('--insertVoiceItem---');
    var arr = JSON.parse(text);
    var voiceHtml = transHtml(arr, true);
    var oA = document.createElement('div');
    oA.className = 'li voiceBox';
    oA.contentEditable = false;
    oA.setAttribute('jsonKey', text);
    oA.innerHTML = voiceHtml;

    var tmpNode = document.createElement("div");
    tmpNode.appendChild(oA.cloneNode(true));
    var str = '<p><br></p>' + tmpNode.innerHTML + '<p><br></p>';

    // $('#summernote').summernote('saveRange');
    // $('#summernote').summernote('insertNode', oA);
    // $('#summernote').summernote('restoreRange');
    document.execCommand('insertHTML', false, str);
    removeNullP()
}

/**
 * 移除无内容p标签
 * @date 2021-08-19
 * @returns {any}
 */
function removeNullP() {
    $('p').each((index, item) => {
        if (item.innerHTML === '') {
            $(item).remove();
        }
    })

}

/**
 * 切换播放状态
 * @date 2021-08-19
 * @param {string} state 0,播放中，1暂停中，2.结束播放
 * @returns {any}
 */
function toggleState(state) {
    console.log('---toggleState--', state);
    if (state == '0') {
        $('.voicebtn').removeClass('pause').addClass('play');
        activeVoice.removeClass('play').addClass('pause');

        voicePlay(true);
    } else if (state == '1') {
        activeVoice.removeClass('pause').addClass('play');
        voicePlay(false);
    }
    else {
        activeVoice.removeClass('pause').addClass('play');
        activeVoice.removeClass('now');
        activeVoice = null;
        voicePlay(false);
    }

    enableSummerNote();
}

/**
 * 设置整个html内容
 * @date 2021-08-19
 * @param {string} html
 * @returns {any}
 */
function setHtml(html) {
    initFinish = false;
    $('#summernote').summernote('code', html);
    initFinish = true;
    changeColor(1);
    // 搜索功能
    webobj.jsCallSetDataFinsh();
}

//设置录音转文字内容 flag: 0: 转换过程中 提示性文本（＂正在转文字中＂)１:结果 文本,空代表转失败了
function setVoiceText(text, flag) {
    console.log('----voice text----');
    if (activeTransVoice) {
        if (flag) {
            if (text) {
                activeTransVoice.find('.translate').html('<div>' + text + '</div>');
                webobj.jsCallTxtChange();
            }
            else {
                activeTransVoice.find('.translate').html('');
            }
            //将转文字文本写到json属性里
            var jsonValue = activeTransVoice.attr('jsonKey');
            var jsonObj = JSON.parse(jsonValue);
            jsonObj.text = text;
            activeTransVoice.attr('jsonKey', JSON.stringify(jsonObj));

            webobj.jsCallTxtChange();
            activeTransVoice = null;
            bTransVoiceIsReady = true;
        }
        else {
            activeTransVoice.find('.translate').html('<p class="noselect">' + text + '</p>');
            bTransVoiceIsReady = false;
        }
    }
    enableSummerNote();
}

//json串拼接成对应html串 flag==》》 false: h5串  true：node串
function transHtml(json, flag) {
    //将json内容当其属性与标签绑定
    var strJson = JSON.stringify(json);
    json.jsonValue = strJson;
    var template;
    if (flag) {
        template = Handlebars.compile(nodeTpl);
    }
    else {
        template = Handlebars.compile(h5Tpl);
    }
    var retHtml = template(json);
    return retHtml;
}

//设置summerNote编辑状态 
function enableSummerNote() {
    if (activeVoice || (activeTransVoice && !bTransVoiceIsReady)) {
        $('#summernote').summernote('disable');
    }
    else {
        $('#summernote').summernote('enable');
    }
}

// 录音播放控制， bIsPaly=ture 表示播放。
function voicePlay(bIsPaly) {
    clearInterval(voiceIntervalObj);
    $('.wifi-circle').removeClass('first').removeClass('second').removeClass('third').removeClass('four');

    if (bIsPaly) {
        var index = 0;
        voiceIntervalObj = setInterval(function () {
            if (activeVoice && activeVoice.hasClass('pause')) {
                var voiceObj = activeVoice.parent().find('.wifi-circle');
                index++;
                switch (index) {
                    case 1:
                        voiceObj.removeClass('four').addClass('first');
                        break;
                    case 2:
                        voiceObj.removeClass('first').addClass('second');
                        break;
                    case 3:
                        voiceObj.removeClass('second').addClass('third');
                        break;
                    case 4:
                        voiceObj.removeClass('third').addClass('four');
                        index = 0;
                        break;
                }
            }
        }, 400);
    }
}

/**
 * 右键功能
 * @date 2021-08-19
 * @param {any} e
 * @returns {any} 
 * type: 0图片 1语音 2文本
 */
function rightClick(e) {
    let type = null;
    let json = null;
    let x = e.clientX
    let y = e.clientY
    if (e.target.tagName == 'IMG') {
        // 图片右键
        type = 0;
        let imgUrl = $(e.target).attr('src')
        let img = e.target
        img.focus();
        // 设置选区
        var range = document.createRange();
        range.selectNode(img);
        // range.collapse(true);
        var sel = window.getSelection();
        if (sel.anchorOffset == 0) {
            sel.removeAllRanges();
            sel.addRange(range);
        };
        json = imgUrl

    } else if ($(e.target).hasClass('demo') || $(e.target).parents('.demo').length != 0) {
        // 语音右键
        type = 1;
        json = $(e.target).parents('.li:first').attr('jsonKey')
        // 当前没有语音在转文字时， 才可以转文字
        if (bTransVoiceIsReady) {
            console.log(activeTransVoice)
            activeTransVoice = $(e.target).parents('.li:first');
        }
        // $('#summernote').summernote('airPopover.rightUpdate')
        $('#summernote').summernote('airPopover.hide')
        var sel = window.getSelection();
        sel.removeAllRanges();

    } else {
        // 文本右键
        json = ''
        type = 2;

    }
    webobj.jsCallPopupMenu(type, x, y, json);
    // 阻止默认右键事件
    // e.preventDefault()
}

/**
 * 深色浅色变换
 * @date 2021-08-19
 * @param {any} flag 1浅色 2深色
 * @returns {any}
 */
function changeColor(flag) {
    if (flag == 1) {
        $('body').css({
            'background': 'rgba(255,255,255,1)',
            "color": 'rgba(65,77,104,1)'
        })
        $('.title').css({
            "color": 'rgba(0,26,46,1)'
        })
        $('.time').css({
            "color": 'rgba(65,77,104,1)'
        })
        $('.li').css({
            "background": 'rgba(0,0,0,0.05)'
        })
        $('.minute').css({
            "color": 'rgba(138,161,180,1)'
        })
        $('.note-popover .popover-content').css({
            "background": 'rgba(247,247,247,1)'
        })
        $('.note-current-fontsize').css({
            "color": 'rgba(65,77,104,1)'
        })
        $('.note-icon-caret').css({
            "color": 'rgba(65,77,104,1)'
        })
        $('.dropdown-menu').css({
            "background": 'rgba(247,247,247,1)'
        })
        $('.colorFont').css({
            "color": 'rgba(65,77,104,1)'
        })
        changeIconColor('darkColor');
    } else {
        $('body').css({
            'background': 'rgba(40,40,40,1)',
            "color": 'rgba(192,198,212,1)'
        })
        $('.title , .time').css({
            "color": 'rgba(192,198,212,1)'
        })
        $('.li').css({
            "background": 'rgba(255,255,255,0.05)'
        })
        $('.minute').css({
            "color": 'rgba(109,124,136,1)'
        })
        $('.note-popover .popover-content').css({
            "background": 'rgba(42,42,42,1)'
        })
        $('.note-current-fontsize').css({
            "color": 'rgba(197,207,224,1)'
        })
        $('.note-icon-caret').css({
            "color": 'rgba(197,207,224,1)'
        })
        $('.note-popover .popover-content i').css({
            "color": '#C5CFE0'
        })
        $('.dropdown-menu').css({
            "background": 'rgba(42,42,42,1)'
        })
        $('.colorFont').css({
            "color": 'rgba(192,198,212,1)'
        })
        $('.dropdown-fontsize li a').css({
            "color": 'rgba(192,198,212,1)'
        })
        changeIconColor('lightColor');
    }

}

/**
 * 改变icon图标颜色
 * @date 2021-08-19
 * @param {any} color
 * @returns {any}
 */
function changeIconColor(color) {
    let iconList = ['icon-strikethrough', 'icon-bold', 'icon-italic', 'icon-underline', 'icon-forecolor', 'icon-backcolor', 'icon-ul', 'icon-ol']
    iconList.forEach((item, index) => {
        if (item == 'icon-forecolor') {
            $('.' + item + ' .path3').addClass(color)
        } else if (item == 'icon-backcolor') {
            $('.' + item + ' .path1,.path2,.path3,.path4').addClass(color)
        }
        else {
            $('.' + item).addClass(color)
        }
    })
}

/**
 * 插入图片
 * @date 2021-08-19
 * @param {any} urlStr 图片地址list
 * @returns {any}
 */
function insertImg(urlStr) {
    urlStr.forEach((item, index) => {
        $("#summernote").summernote('insertImage', item, 'img');
    })
}

