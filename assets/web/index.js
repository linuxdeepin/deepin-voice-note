//C++ 调用js接口
//信号绑定
// initData(const QString& jsonData); 初始化，参数为json字符串
// void setHtml(const QString& html); 初始化，设置html
// insertVoiceItem(const QString &jsonData);　插入语音，参数为json字符串

//callback回调
// const QString getHtml();获取整个html
// const QString getAllNote();获取所有语音列表的Json
//


//js 调用c++接口


var h5Tpl  = `
<div class="li" contenteditable="false" jsonKey="{{jsonValue}}">
    <div>
    <div class="demo" >
        <div class="left">
            <div class="voiceBtn play"></div>
        </div>
        <div class="right">
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
            <div class="transBtn"></div>
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
    <div>
    <div class="demo" >
        <div class="left">
            <div class="voiceBtn play"></div>
        </div>
        <div class="right">
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
            <div class="transBtn"></div>
        </div>
    </div>
    <div class="translate">
        {{#if text}}
        <div>{{text}}</div>
        {{/if}}
    </div></div>`;

var webobj;    //js与qt通信对象
var activeVoice = null;  //当前正操作的语音对象
var activeTransVoice = null;  //执行语音转文字对象
var bTransVoiceIsReady = true;  //语音转文字是否准备好
var initFinish = false;
var voiceIntervalObj;    //语音播放动画定时器对象

//设置默认焦点， 不可拖拽， 
$('#summernote').summernote({
    minHeight: null,             // set minimum height of editor
    maxHeight: null,             // set maximum height of editor
    focus: true,                  // set focus to editable area after initializin
    disableDragAndDrop: true,
    shortcuts:false,
    toolbar: [
        ['style', ['style']],
        ['font', ['bold', 'underline', 'clear']],
        ['fontname', ['fontname']],
        ['color', ['color']],
        ['para', ['ul', 'ol', 'paragraph']],
        ['table', ['table']],
        ['insert', ['link', 'picture', 'video']],
    ]
});

//设置全屏模式
$('#summernote').summernote('fullscreen.toggle');

//捕捉change事件
$('#summernote').on('summernote.change', function(we, contents, $editable) {
    if (webobj && initFinish)
    {
        console.log('---------->change');
        webobj.jsCallTxtChange();
    }
});

//点击变色
$('body').on('click', '.li', function (e) {
    console.log('div click...');
    e.stopPropagation();
    $('.li').removeClass('active');
    $(this).addClass('active');
})

//播放
$('body').on('click', '.voiceBtn', function (e) {
    console.log('------playBtn click...');
    // e.stopPropagation();
    var curVoice = $(this).parents('.li:first');
    var jsonString = curVoice.attr('jsonKey');
    var bIsSame = $(this).hasClass('now');
    var curBtn = $(this);
    $('.voiceBtn').removeClass('now');
    activeVoice = curBtn;
    activeVoice.addClass('now');
    
    webobj.jsCallPlayVoice(jsonString, bIsSame, function (state) {
        //TODO 录音错误处理
    });
})

//语音转文字按钮
$('body').on('click', '.transBtn', function (e) {
    console.log('------transBtn click...');
    // e.stopPropagation();
    var jsonString = $(this).parents('.li:first').attr('jsonKey');
    webobj.jsCallPopVoiceMenu(jsonString);
    // 当前没有语音在转文字时， 才可以转文字
    if (bTransVoiceIsReady)
    {
        activeTransVoice = $(this).parents('.li:first');
    }
})

//获取整个处理后Html串,去除所有标签中临时状态
function getHtml(){
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
function getAllNote(){
    var jsonObj = {};
    var jsonArray = [];
    var jsonString;
    $('.li').each(function() {
        jsonString = $(this).attr('jsonKey');
        jsonArray[jsonArray.length] = JSON.parse(jsonString);     
    })
    jsonObj.noteDatas = jsonArray;
    var retJson = JSON.stringify(jsonObj);

    console.log('========>',retJson);
    return retJson;
}

//当前记事本是否有语音
function bHaveNote(){
    var noteList  = $('.li');
    var bFlag = false;
    if (noteList.length > 0)
    {
        bFlag = true;
    }
    return bFlag;
}

//获取当前选中录音json串
function getActiveNote(){
    var retJson = '';
    if ($('.active').length > 0)
    {
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

        //通知QT层完成通信绑定
        webobj.jsCallChannleFinish();
    }
)

//初始化数据 
function initData(text) {
    initFinish = false;
    console.log('=============>',text);
    var arr = JSON.parse(text);
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
            voiceHtml = transHtml(item,false);
            html += voiceHtml;
        }
    })

    $('#summernote').summernote('code', html);
    initFinish = true;
}

//录音插入数据
function insertVoiceItem(text) {
    console.log('--insertVoiceItem---',text);
    
    var arr = JSON.parse(text);
    var voiceHtml = transHtml(arr,true);
    var oA=document.createElement('div');
    oA.className='li';
    oA.contentEditable = false;
    oA.setAttribute('jsonKey', text);
    oA.innerHTML=voiceHtml;
    $('#summernote').summernote('saveRange');
	$('#summernote').summernote('insertNode', oA);
    $('#summernote').summernote('restoreRange');
}

//播放状态，0,播放中，1暂停中，2.结束播放
function toggleState(state) {
    console.log('---toggleState--',state);
    if (state == '0') {
        $('.voiceBtn').removeClass('pause').addClass('play');
        activeVoice.removeClass('play').addClass('pause');

        voicePlay(true);
    } else if (state == '1') {
        activeVoice.removeClass('pause').addClass('play');
        voicePlay(false);
    }
    else{
        activeVoice.removeClass('pause').addClass('play');
        activeVoice.removeClass('now');
        activeVoice = null;
        voicePlay(false);
    }

    enableSummerNote();
}

//设置整个html内容
function setHtml(html){
    initFinish = false;
    console.log('--setHtml---');
    $('#summernote').summernote('code',html);
    initFinish = true;
}

//设置录音转文字内容 flag: 0: 转换过程中 提示性文本（＂正在转文字中＂)１:结果 文本,空代表转失败了
function setVoiceText(text,flag){
    console.log('----voice text----');
    if (activeTransVoice)
    {
        if (flag)
        {
            if (text)
            {
                activeTransVoice.find('.translate').html('<div>'+text+'</div>');
                webobj.jsCallTxtChange();
            }
            else
            {
                activeTransVoice.find('.translate').html('');
            }
            //将转文字文本写到json属性里
            var jsonValue = activeTransVoice.attr('jsonKey');
            var jsonObj = JSON.parse(jsonValue);
            jsonObj.text = text;
            activeTransVoice.attr('jsonKey',JSON.stringify(jsonObj));

            webobj.jsCallTxtChange();
            activeTransVoice = null;
            bTransVoiceIsReady = true;
        }
        else
        {
            activeTransVoice.find('.translate').html('<p class="noselect">'+text+'</p>');
            bTransVoiceIsReady = false;
        }
    }
    enableSummerNote();
}

//json串拼接成对应html串 flag==》》 false: h5串  true：node串
function transHtml(json,flag){
    //将json内容当其属性与标签绑定
    var strJson = JSON.stringify(json);
    json.jsonValue = strJson;
    var template;
    if (flag)
    {
        template = Handlebars.compile(nodeTpl);
    }
    else
    {
        template = Handlebars.compile(h5Tpl);
    }
    var retHtml =  template(json);
    return retHtml;
}

//设置summerNote编辑状态 
function enableSummerNote(){
    if (activeVoice || (activeTransVoice && !bTransVoiceIsReady))
    {
        $('#summernote').summernote('disable');
    }
    else
    {
        $('#summernote').summernote('enable');
    }
}

// 录音播放控制， bIsPaly=ture 表示播放。
function voicePlay(bIsPaly){
    clearInterval(voiceIntervalObj);
    $('.wifi-circle').removeClass('first').removeClass('second').removeClass('third').removeClass('four');

    if (bIsPaly)
    {
        var index = 0;
        voiceIntervalObj = setInterval(function(){
            if (activeVoice && activeVoice.hasClass('pause'))
            {
                var voiceObj = activeVoice.parent().next().find('.wifi-circle');
                index++;
                switch(index){
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
        },400);
    }  
}
