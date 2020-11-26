var webobj;
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
$('body').on('click', '.left .btn', function (e) {
    
    e.stopPropagation();
    var bId = $(this).attr('data-id');
    var state = $(this).hasClass('play') ? 0 : 1;
    console.log('.....bofang...',state,this);
    // webobj.playButtonClick(bId, state, function (state) {
    //     //item 
    //     toggleState(statse, bId)
    // });
    //播放
     toggleState(state,  $(this))
})

//按钮切换状态 c++调用
function toggleState(state, element) {
    if (state == '0') {
        $('.left .btn').removeClass('pause').addClass('play');
        element.removeClass('play').addClass('pause');
    } else if (state == '1') {
        element.removeClass('pause').addClass('play');
    }
}

function init(type, arr) {
    console.log('=============>',arr);
    $('#summernote').summernote('reset');
    $('#summernote').summernote('focus');
    var tpl = $("#voiceTemplate").html();
    var template = Handlebars.compile(tpl);
    var html = '';
    var voiceHtml;
    var txtHtml;
    arr.noteDatas.forEach((item, index) => {
        //false: txt
        if (item.type == false) {
            txtHtml = '<p>' + item.text +'</p>'
            html += txtHtml;
        }
        // //true: voice
        else{
            voiceHtml = template(item);
            html += voiceHtml
        }
    })

    $('#summernote').summernote('code', html);
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
    console.log('--initdata---',text);
    fnInit(text, 1)
}

new QWebChannel(qt.webChannelTransport,
    function (channel) {
        console.log('qt.webChannelTransport....');
        webobj = channel.objects.webobj;
        //所有的c++ 调用js的接口都需要在此绑定格式，webobj.c++函数名（jscontent.cpp查看.connect(js处理函数)
        //例如 webobj.c++fun.connect(jsfun)
        webobj.initData.connect(initData);
        webobj.switchPlayBtn.connect(toggleState);
        webobj.setHtml.connect(setHtml);
})


function getHtml(){
    // console.log('===html==>',$('#summernote').summernote('code'));
    return $('#summernote').summernote('code');
}


function setHtml(html){
    $('#summernote').summernote('code',html);
}



  var BlockId = 0;
  var createTime = '02.20';
  var title = '语音';

  function btnClick(){
      console.log('btn click...');
    this.BlockId ++;
    var titleTmp = this.title + this.BlockId; 
    var oHML=`
                    <div class="demo" data-id={{this.BlockId}}>
                        <div class="left">
                            <div class="btn play" data-id="{{this.BlockId}}"></div>
                        </div>
                        <div class="right">
                            <div class="lf">
                                <div class="title">${titleTmp}</div>
                                <div class="minute padtop">${this.createTime}</div>
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
                                <div class="time padtop">{{this.voiceSize}}</div>
                        </div>
                    </div>`;
    var oA=document.createElement('div');
    oA.className='li'; 
    oA.contentEditable = false;
    oA.innerHTML=oHML;
    // $('#summernote').summernote('pasteHTML', oHML);
    $('#summernote').summernote('saveRange');
	$('#summernote').summernote('insertNode', oA);
    $('#summernote').summernote('restoreRange');
}