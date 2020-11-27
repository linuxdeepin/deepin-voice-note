//C++ 调用js接口

//信号绑定
// initData(const QString& jsonData); 初始化，参数为json字符串
// void setHtml(const QString& html); 初始化，设置html
// insertVoiceItem(const QString &jsonData);　插入语音，参数为json字符串
//callback回调
// const QString getHtml();获取整个html


//js 调用c++接口



var webobj;    //js与qt通信对象
var curActiveVoiceBtn;  //当前播放的语音对象
$('#summernote').summernote({
    height: 300,                 // set editor height
    minHeight: null,             // set minimum height of editor
    maxHeight: null,             // set maximum height of editor
    focus: true,                  // set focus to editable area after initializin
    airMode: true,
    disableDragAndDrop: true,
    shortcuts:false,
}); 

//点击变色
$('body').on('click', '.li', function (e) {
    console.log('div click...');
    e.stopPropagation();
    $('.li').removeClass('active');
    console.log('=======click...',$('.li'));
    $(this).addClass('active');
})

//播放
$('body').on('click', '.btn', function (e) {
    
    e.stopPropagation();
    var curVoiceBtn = this;
    var curVoice = $(this).parents('.li:first');


    var jsonString = curVoice.attr('jsonKey');
    var bIsSame = curVoiceBtn.isEqualNode(curActiveVoiceBtn);

    console.log('---->',bIsSame);
    console.log('cur json---->',jsonString);

    webobj.jsCallPlayVoice(jsonString, bIsSame, function (state) {
        curActiveVoiceBtn = curVoiceBtn;

        //item 
        // toggleState(statse, bId)
    });
    //播放
})

//按钮切换状态 c++调用
function toggleState(state, element) {
    if (state == '0') {
        $('.btn').removeClass('pause').addClass('play');
        element.removeClass('play').addClass('pause');
    } else if (state == '1') {
        element.removeClass('pause').addClass('play');
    }
}

//type:1 初始化  2 插入
function init(type, arr) {
    console.log('=============>',arr);
    var tpl = $("#voiceTemplate").html();
    var template = Handlebars.compile(tpl);
    var html = '';
    var voiceHtml;
    var txtHtml;
    // if (type == 1){
    //     html += '<p><br></p>';
    // }
    arr.noteDatas.forEach((item, index) => {
        //false: txt
        if (item.type == 1) {
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
            //将json内容当其属性与标签绑定
            var strItem = JSON.stringify(item);
            item.jsonValue = strItem;
            voiceHtml = template(item);
            html += voiceHtml
        }
    })
    html += '<p><br></p>';
    if (type == 1){
        $('#summernote').summernote('code', html);
    }
    else{
        $('#summernote').summernote('pasteHTML', html);
    }
}

function fnInit(text, type) {
    var initText = JSON.parse(text);
    var newText = initText.noteDatas;

    //解决异步更新数据页面接收问题
    //获取转换时间
    function fnAsyncTime(item) {
        return new Promise(function (resolve, reject) {
            webobj.jsCallGetVoiceTime(item.createTime, function (time) {
                // item.createTime = time;
                item.transTime = time;
                resolve();
            });
        })
    }
    //获取转换时长
    function fnAsyncSize(item){
        return new Promise(function (resolve, reject) {
            webobj.jsCallGetVoiceSize(item.voiceSize, function (vSize) {
                // item.voiceSize = vSize;
                item.transSize = vSize;
                resolve()
            })
        })
    }


    Promise.all(
        newText.map(item => {
            return new Promise(async (resolve, reject) => {
                await fnAsyncTime(item);
                await fnAsyncSize(item);
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
    console.log('--initdata---',text);
    fnInit(text, 1)
}

//录音插入数据
var insertVoiceItem = function (text) {
    console.log('--insertVoiceItem---',text);
    fnInit(text, 2);
}

new QWebChannel(qt.webChannelTransport,
    function (channel) {
        console.log('qt.webChannelTransport....');
        webobj = channel.objects.webobj;
        //所有的c++ 调用js的接口都需要在此绑定格式，webobj.c++函数名（jscontent.cpp查看.connect(js处理函数)
        //例如 webobj.c++fun.connect(jsfun)
        webobj.callJsInitData.connect(initData);
        webobj.callJsInsertVoice.connect(insertVoiceItem);
        // webobj.switchPlayBtn.connect(toggleState);
        webobj.callJsSetHtml.connect(setHtml);
})

function getHtml(){
    return $('#summernote').summernote('code');
}

function setHtml(html){
    console.log('--setHtml---',html);
    $('#summernote').summernote('code',html);
}