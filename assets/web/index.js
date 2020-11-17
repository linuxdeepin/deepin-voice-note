var webobj;
var initData=function(text)
{
    init(text)
       //alert("init" + text);
}
var insertVoiceItem=function(text)
{
       alert("insert voice:" + text);
}
new QWebChannel(qt.webChannelTransport,
   function(channel){
    webobj = channel.objects.webobj;
    window.foo = webobj;
    webobj.initData.connect(initData);
    webobj.insertVoiceItem.connect(insertVoiceItem);
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
    function readyEditor(id) {
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
    }
    readyEditor('summernote');
    function fnClick(id) {
        i++;
        b = i;
        var str = 'summernote' + b;
        var oHML = `
    <div class="li">
    <div class="demo" forder_id="hello-world">
    <div class="left">
        <div class="btn play"></div>
        </div>
        <div class="right">
            <div class="lf">
                <div class="title">语音${b}</div>
                <div class="minute padtop">1分钟前</div>
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
                <div class="time padtop">12:01</div>
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
        var range = window.getSelection().getRangeAt(0);
        var srcele = range.selectionStart;//获取到当前元素
        console.log(range)
    }
    //播放
    $('body').on('click', '.left .btn', function (e) {
        e.stopPropagation();
        if($(this).hasClass('play')){
            $('.left .btn').removeClass('pause').addClass('play');
            $(this).addClass('pause').removeClass('play');
        }else{
            //$('.left .btn').removeClass('play').addClass('pause');
            $(this).removeClass('pause').addClass('play');
        }
    })
    //点击变色
    $('body').on('click', '.li', function (e) {
        e.stopPropagation();
        $(this).addClass('active').siblings('.li').removeClass('active');
    })
    //转换文本
    $('body').on('click', '.time', function (e) {
        e.stopPropagation();
        $(this).parents('.li').find('.translate').html('<p>语音转文本.....</p>');
        var that = $(this);
        setTimeout(function () {
            that.parents('.li').find('.translate').html('<div class="">转换成功</div>');
        }, 1000)
    })
    $('body').on('click', function () {
        $('.li').removeClass('active');
    })
    //删除
    $('body').on('click', '.title', function () {
        var a = confirm('是否删除');
        if (a) {
            $(this).parents('.li').remove();
        } else {
            return
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
    var arr=[];
    //初始化
    function init(arr){
        alert(1)
        console.log(arr);
        var tpl  =  $("#main").html();
        var template = Handlebars.compile(tpl);
        var html = template(arr);
        $(body).html(html);
    } 
