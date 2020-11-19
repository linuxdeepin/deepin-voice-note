//C++ 调用js接口

// initData(const QString& jsonData); 初始化，参数为json字符串
// insertVoiceItem(const QString &jsonData);　插入语音，参数为json字符串
// switchPlayBtn(const QString& id, int status);设置播放按钮，id为创建插件传入的ｉｄ，state 为１，０，分别代表显示播放按钮，暂停按钮

//js 调用c++ 函数
//通知播放按钮被按下，id为创建插件传入的ｉｄ，state 为０，１，分别代表播放按钮按下，暂停按钮按下, 返回值１代表成功，０代表失败
//playButtonClick(const QString& id, int status);
//getVoiceSize(qint64 millisecond); 参数数字型,单位毫秒，例如1000,返回格式化后的字符串;
//getVoiceTime(const QString &time);语音插件创建时间格式化,参数字符串，例如"2020-10-20 16:23:44"，返回格式化后的字符串
//rightMenuClick(const QString& id, int select); 鼠标右键单击，　参数：id为创建插件传入的ｉｄ,select 1 此插件有选中内容，０没有选中内容
var webobj;
//初始化
function init(type, arr) {
    var tpl = $("#main").html();
    var template = Handlebars.compile(tpl);
    var html = template(arr);
    if (type == 1) {
        $('.main').html(html);
    } else {
        $('.main').append(html);
    }
    arr.noteDatas.forEach((item, index) => {
        if (item.type == false) {
            readyEditor('summernote' + item.BlockId, item.text);
        }
    })
}
function fnInit(text, type) {
    var initText = JSON.parse(text);
    var newText = initText.noteDatas;
    newText.forEach(item => {
        item.type = item.type == 1 ? false : true
    })
    //解决异步更新数据页面接收问题
    function fnAsync(item) {
        return new Promise(function (resolve, reject) {
            webobj.getVoiceTime(item.createTime, function (time) {
                item.createTime = time;
                resolve();
            });
            webobj.getVoiceSize(item.voiceSize, function (vSize) {
                item.voiceSize = vSize;
                resolve()
            })
        })
    }
    Promise.all(
        newText.map(item => {
            return new Promise(async (resolve, reject) => {
                await fnAsync(item)
                resolve()
            })
        })
    ).then(() => {
        initText.noteDatas = newText;
        init(type, initText)
    })
}
//初始化数据
var initData = function (text) {
    fnInit(text, 1)
}
//录音插入数据
var insertVoiceItem = function (text) {
    fnInit(text, 2);
}
new QWebChannel(qt.webChannelTransport,
    function (channel) {
        webobj = channel.objects.webobj;
        window.foo = webobj;
        //所有的c++ 调用js的接口都需要在此绑定格式，webobj.c++函数名（jscontent.cpp查看.connect(js处理函数)
        //例如 webobj.c++fun.connect(jsfun)
        webobj.initData.connect(initData);
        webobj.insertVoiceItem.connect(insertVoiceItem);
        webobj.switchPlayBtn.connect(toggleState);
        webobj.setVoiceToText.connect(transText);
    })
//DOM对象转换为string
if (!document.HTMLDOMtoString) {
    document.HTMLDOMtoString = function (HTMLDOM) {
        const div = document.createElement("div")
        div.appendChild(HTMLDOM)
        return div.innerHTML
    }
}
var i = 0;
var b = 0;
var currentId = 0;
function readyEditor(id, text) {
    $('#' + id + '').summernote({
        airMode: true,
        disableDragAndDrop: true,
        focus: true,
        callbacks: {
            onFocus: function () {
                //var text=document.HTMLDOMtoString(this);
                var text = $(this).attr('data-id');
                currentId = text;
            },
            onChange: function () {
                // console.log(document)
                // var range = document.selection.createRange();
                // var srcele = range.parentElement();//获取到当前元素
                //console.log(srcele)
            }
        }
    });
    $('#' + id + '').summernote('code', text)
}
readyEditor('summernote');
function fnClick(str) {
    var str = 'summernote' + str.BlockId;
    var oHML = `
    <div class="li">
    <div class="demo" forder_id="hello-world">
    <div class="left">
        <div class="btn play"></div>
        </div>
        <div class="right">
            <div class="lf">
                <div class="title">${str.title}</div>
                <div class="minute padtop">${str.createTime}</div>
            </div>
            <div class="lr">
                <div class="icon">
                <div class="wifi-symbol">
                    <div class="wifi-circle first"></div>
                    <div class="wifi-circle second"></div>
                    <div class="wifi-circle third"></div>
                    <div class="wifi-circle four"></div>
                </div>
                </div>
                <div class="time padtop">${str.voiceSize}</div>
            </div>
        </div>
    </div><div class="translate"></div>
    </div>
    <div id="${str}" data-id="${b}">
    </div>`;
    var oA = document.createElement('a');
    oA.className = 'list';
    oA.id = 'item' + b;
    oA.setAttribute('data-id', b)
    //oA.contentEditable = false;
    oA.innerHTML = oHML;
    if ($('.list').length > 1) {
        $('.main').find('.list[data-id="' + currentId + '"]').after(oA.innerHTML);
    } else {
        $('.main').append(oA.innerHTML);
    }

    readyEditor(str)
    // var range = window.getSelection().getRangeAt(0);
    // var srcele = range.selectionStart;//获取到当前元素
    // console.log(range)
}
//播放
$('body').on('click', '.left .btn', function (e) {
    e.stopPropagation();
    var bId = $(this).attr('data-id');
    var state = $(this).hasClass('play') ? 0 : 1;
    webobj.playButtonClick(bId, state, function (state) {
        //item 
        toggleState(state, bId)
    });
    //播放
})
//按钮切换状态 c++调用
function toggleState(state, item) {
    if (state == '1') {
        $('.left .btn').removeClass('pause').addClass('play');
        $('.left .btn[data-id=' + item + ']').addClass('pause').removeClass('play');
    } else if (state == '0') {
        $('.left .btn[data-id=' + item + ']').removeClass('pause').addClass('play');

    }
}
//点击变色
$('body').on('click', '.li', function (e) {
    e.stopPropagation();
    $(this).addClass('active').siblings('.li').removeClass('active');
})
//转换文本
// $('body').on('click', '.time', function (e) {
//     e.stopPropagation();
//     $(this).parents('.li').find('.translate').html('<p>语音转文本.....</p>');
//     var that = $(this);
//     setTimeout(function () {
//         that.parents('.li').find('.translate').html('<div class="">转换成功</div>');
//     }, 1000)
// })
//转换文本
function transText(id, text, state) {
    if (state) {
        $('.li[data-id=' + id + ']').find('.translate').text(text);
    }else{
        $('.li[data-id=' + id + ']').find('.translate').html(text);
    }
}
//删除
function deleteItem(id){
    $('.li[data-id=' + id + ']').remove();
    $('.li[data-id=' + id + ']').next().remove();
}

$('body').on('click', function () {
    $('.li').removeClass('active');
})
//右键操作
$(document).on('contextmenu', '.li,.note-editor', function (e) {
    //e.preventDefault();
    var oId = '';
    var text = 0;
    if (3 == e.which) {
        //alert($(this).attr('data-id'))
        //列表区域
        if ($(this).hasClass('li')) {
            oId = $(this).attr('data-id');
            text = 0;
        } else {
            //文本区域
            text = getSelectedHtml() ? 1 : 0;
            oId = $(this).prev().attr('data-id');
        }
        webobj.rightMenuClick(oId, text)
    }
})

document.ondrop = function (event) {
    return false;
};
document.ondragenter = function (event) {
    return false;
};
document.ondragover = function (event) {
    return false;
};
//获取选中内容
function getSelectedHtml() {
    var selectedHtml = "";
    var documentFragment = null;
    try {
        if (window.getSelection) {
            documentFragment = window.getSelection().getRangeAt(0).cloneContents();
        } else if (document.selection) {
            documentFragment = document.selection.createRange().HtmlText;
        }

        for (var i = 0; i < documentFragment.childNodes.length; i++) {
            var childNode = documentFragment.childNodes[i];
            if (childNode.nodeType == 3) {  // Text 节点
                selectedHtml += childNode.nodeValue;
            } else {
                var nodeHtml = childNode.outerHTML;
                selectedHtml += nodeHtml;
            }

        }

    } catch (err) {

    }

    return selectedHtml;
}
function themeColor(color) {
    //$('.left').css('background-color',color);
    var nod = document.createElement('style'),
        str = `
                .btn{
                    background-color:rgba(${color},1);
                }
                .li.active{
                    background-color:rgba(${color},.5);
                }
            `;
    nod.type = 'text/css';
    nod.innerHTML = str; //或者写成 nod.appendChild(document.createTextNode(str))  
    document.getElementsByTagName('head')[0].appendChild(nod);
}
themeColor('237,86,86');

