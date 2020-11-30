//C++ 调用js接口

//信号绑定
// initData(const QString& jsonData); 初始化，参数为json字符串
// void setHtml(const QString& html); 初始化，设置html
// insertVoiceItem(const QString &jsonData);　插入语音，参数为json字符串
//callback回调
// const QString getHtml();获取整个html


//js 调用c++接口


var h5Tpl  = `
<div class="li" contenteditable="false" jsonKey="{{jsonValue}}">	
    <div class="demo" >
        <div class="left">
        <div class="btn play"></div>
        </div>
        <div class="right">
            <div class="lf">
                <div class="title">{{title}}</div>
                <div class="minute padtop">{{createTime}}</div>
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
                <div class="time padtop">{{transSize}}</div>
            </div>
            <div class="transBtn"></div>
        </div>
    </div>
    <div class="translate">
        {{#if text}}
        <div>{{text}}</div>
        {{/if}}
    </div>
</div>`;

var webobj;    //js与qt通信对象
var activeVoice;  //当前正操作的语音对象
var activeTransVoice; 

$('#summernote').summernote({
    // height: 300,                 // set editor height
    minHeight: null,             // set minimum height of editor
    maxHeight: null,             // set maximum height of editor
    focus: true,                  // set focus to editable area after initializin
    // airMode: true,
    disableDragAndDrop: true,
    shortcuts:false,
}); 

//点击变色
$('body').on('click', '.li', function (e) {
    console.log('div click...');
    e.stopPropagation();
    $('.li').removeClass('active');
    $(this).addClass('active');
})

//播放
$('body').on('click', '.btn', function (e) {
    
    e.stopPropagation();
    var curVoice = $(this).parents('.li:first');
    var jsonString = curVoice.attr('jsonKey');
    var bIsSame = $(this).hasClass('now');
    var curBtn = $(this);
    $('.btn').removeClass('now');
    activeVoice = curBtn;
    activeVoice.addClass('now');
    
    webobj.jsCallPlayVoice(jsonString, bIsSame, function (state) {
        //TODO 录音错误处理
    });
})

//语音转文字按钮
$('body').on('click', '.transBtn', function (e) {
    console.log('------trans click...');
    var jsonString = $(this).parents('.li:first').attr('jsonKey');
    webobj.jsCallPopVoiceMenu(jsonString);
    activeTransVoice = $(this).parents('.li:first');
})

//播放状态，0,播放中，1暂停中，2.结束播放
function toggleState(state) {
    console.log('========.');
    if (state == '0') {
        $('.btn').removeClass('pause').addClass('play');
        activeVoice.removeClass('play').addClass('pause');
    } else if (state == '1') {
        activeVoice.removeClass('pause').addClass('play');
    }
    else{
        activeVoice.removeClass('pause').addClass('play');
        $('.btn').removeClass('now');
    }
}

function getHtml(){
    return $('#summernote').summernote('code');
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
})

//初始化数据 
function initData(text) {
    var arr = JSON.parse(text);
    console.log('=============>',arr);
    var html = '';
    var voiceHtml;
    var txtHtml;

    arr.noteDatas.forEach((item, index) => {
        //false: txt
        if (item.type == 1) {
            console.log('---txt---');
            if (item.text == '')
            {
                txtHtml = '<p><br></p>';
            }
            else{
                txtHtml = '<p>' + item.text +'</p>';
            }
            html += txtHtml;
        }
        //true: voice
        else{
            console.log('---voice---');
            voiceHtml = transHtml(item);
            html += voiceHtml;
        }
    })

    $('#summernote').summernote('code', html);
}

//录音插入数据
function insertVoiceItem(text) {
    console.log('--insertVoiceItem---',text);
    var arr = JSON.parse(text);
    var voiceHtml = transHtml(arr);
    $('#summernote').summernote('pasteHTML', voiceHtml);
}

//设置整个html内容
function setHtml(html){
    console.log('--setHtml---',html);
    $('#summernote').summernote('code',html);
}

//设置录音转文字内容 flag: 0: 转换过程中 提示性文本（＂正在转文字中＂)１:结果 文本,空代表转失败了
function setVoiceText(text,flag){
    console.log('----voice text----');
    // activeTransVoice.
    if (activeTransVoice)
    {
        if (flag)
        {
            if(!text) return;
        activeTransVoice.find('.translate').html('<div>'+text+'</div>');
        }
        else
        {
            activeTransVoice.find('.translate').html('<p class="noselect">'+text+'</p>');
        }
    }
}

function transHtml(json){
    //将json内容当其属性与标签绑定
    var strItem = JSON.stringify(json);
    json.jsonValue = strItem;
    var template = Handlebars.compile(h5Tpl);
    var retHtml =  template(json);
    return retHtml;
}