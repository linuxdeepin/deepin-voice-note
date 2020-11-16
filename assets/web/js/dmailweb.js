//
//

function connectQtObject(callback)
{
    if (typeof(window.view_bridge) != "undefined" && window.view_bridge.onWebMessage != "undefined")
    {
        typeof(callback) === "function" && callback();
        return;
    }
    new QWebChannel(qt.webChannelTransport,function (channel)  {
        window.view_bridge = channel.objects.ViewContext;
        window.view_bridge.onViewMessage.connect(function (response) {
            toLocalImgSrc(response);
        });
        typeof(callback) === "function" && callback();
    });
}

function submit(imgmaps)
{
    toInLineImgSrc(imgmaps);
    connectQtObject(function() {
        window.view_bridge.onWebMessage("submit", $('#summernote').summernote('code'));
        toLocalImgSrc(imgmaps);
    });
}
function toolbarShow()
{
  $('#summernote').summernote('toolbar.showItem');

}
function toolbarHide()
{
  $('#summernote').summernote('toolbar.hidenItem');
}
function insertImageToHTML(img)
{
    $('#summernote').summernote('editor.insertImage',img);
}
function removeSum(){
    $('#summernote').summernote('code','');
}
function webIsEmpty()
{
    if ($('#summernote').summernote('isEmpty'))
    {
        return false;
    }
    else
    {
        return true;
    }
}

//function htmlSum(){
////   alert($('#summernote').summernote('code').toString());
//    connectQtObject(function() {
//        window.view_bridge.onWebMessage("htmlSum", '<div id="sign">'+$('#summernote').summernote('code')+'</div>');
////        toLocalImgSrc(imgmaps);
//    });
////    return "123";
//}

function htmlSum(){
    var str;
    if($('#summernote').summernote('code').indexOf('id="sign"')>-1){
        str=$('#summernote').summernote('code');

    }else{
         str='<div id="sign">'+$('#summernote').summernote('code')+'</div>'
    }
    connectQtObject(function() {
        window.view_bridge.onWebMessage("htmlSum",str);
    });
}

function htmlSumSave(){
    var str;
    if($('#summernote').summernote('code').indexOf('id="sign"')>-1){
        str=$('#summernote').summernote('code');

    }else{
         str='<div id="sign">'+$('#summernote').summernote('code')+'</div>'
    }
    connectQtObject(function() {
        window.view_bridge.onWebMessage("htmlSumSave",str);
    });
}
function changeTheme(colorString)
{
    //$('#summernote').summernote('editor.changeMailTheme',colorString);
    // var textColor=(colorString=='#252525')?'#ffffff':'#252525';
    // $('.note-editable').css({'backgroundColor':colorString,'color':textColor});
    // $('.modal-content,.modal-content .form-control').css({'backgroundColor':colorString,'color':textColor})
    // var divs =  document.querySelectorAll("div");
    // document.body.style.background = colorString;
    // document.body.style.color = (colorString=='#252525')?'#ffffff':'#252525';
     //$('.note-status-output').css({'backgroundColor':colorString,'color':textColor});
     //$('#summernote').summernote('toolbar.changeToolbarThemeColor',colorString);

}
//粘贴按钮纯文本
function paste(result){
      console.log(result)
    //return;
    $('#summernote').summernote('insertText', result);
}
//粘贴按钮html
function pastehtml(result){
   //$('#summernote').summernote('editor.pasteHTML', result);
    //走浏览器自身行为
    document.execCommand('insertHTML',false,result);
}
//粘贴图片
function pasteImag(base64){
    //return;
   $('#summernote').summernote('editor.insertImage',base64);
}


//type区分编辑和转发 true编辑
function setMailSign(text,type)
{
    var index=text.indexOf('sign');
    var newtext=text.slice(0,index+6)+"<hr/>"+text.slice(index+6);
    var ishsow=$('.note-editor').find('#sign').length;
    if($('.note-editor').find('#MailBelowDiv').length===0){
        var signDiv='<div id="MailBelowDiv"><div>'+$('#summernote').summernote('code')+'</div></div><br><br>';
        $('#summernote').summernote('code',signDiv+newtext)
    }
    else{
        if(text){
            if(ishsow==0){
                //$('.note-editor').find('#MailBelowDiv').next().append(newtext)
                if(type=='true'){
                    $('.note-editable').append(newtext)
                }else{
                    $('.note-editor').find('#MailBelowDiv').next().find('br').eq(1).after(newtext);
                }
            }else{
                    $('.note-editor').find('#sign').html(newtext)
                }
            }else{
                $('.note-editor').find('#sign').html('')
            }
        }


};
function tabSign(text){
    $('#summernote').summernote('code',text);
}

function startCheckImgs(webhome)    
{
    window.webhome = webhome;
//    connectQtObject(function() {
//        var cids = getImgCIDs();
//        window.view_bridge.onWebMessage("HtmlImgMaps", cids);
//    });
}
function getImgCIDs() {
    var cidlist;
    var imgs = document.getElementsByTagName("img");
    for(i = 0;i < imgs.length; i++) {
        var src = imgs[i].currentSrc.replace(/(^\s*)/g,"");
        var ind = src.indexOf(':');
        if (src.substr(0, ind).toLowerCase() != "cid")
        {
            continue;
        }
        cidlist += src.substr(ind + 1) + ";";
    }
    return cidlist;
}

function toLocalImgSrc(imgmaps) {
    var srclst = imgmaps.split(";");
    var imgs = document.getElementsByTagName("img");
    for(i = 0; i < imgs.length; i++) {
        var src = imgs[i].src.replace(/(^\s*)/g,"");
        var ind = src.indexOf(":");
        if (ind == -1 || src.substr(0, ind).toLowerCase() != "cid")
        {
            continue;
        }
        var cid = src.substr(ind + 1);
        for(j = 0; j < srclst.length; j+=2) {
            if (srclst[j] == cid) {
                imgs[i].src= "images/" + srclst[j+1];
                break;
            }
        }
    }
}
function toInLineImgSrc(imgmaps) {
    var srclst = imgmaps.split(";");
    var imgs = document.getElementsByTagName("img");
    for(i = 0; i < imgs.length; i++) {
        var src = imgs[i].currentSrc.replace(/(^\s*)/g,"");
        var ind = src.lastIndexOf("/");
        if (ind == -1 || src.substr(0, ind).indexOf(window.webhome + "/images") == -1)
        {
            continue;
        }console.log("1111ddddgy");
        console.log(imgmaps)
        var imgname = src.substr(ind + 1);
        console.log(imgname)
        for(j = 0; (j + 1) < srclst.length; j+=2) {
            ind = srclst[j + 1].lastIndexOf("/");
            src = srclst[j + 1].substr(ind + 1);
            if (src == imgname) {
                imgs[i].src= "cid:" + srclst[j]; console.log("1111fdddddddfff11gddddddddgy");
                console.log(srclst[j]);
                break;
            }
        }
    }
}

function removeSum(){$("#summernote").summernote("code",'');}
function replayHtml(str){
    var html="";
    var arr='';
    if(str){
        arr=str.split(',');
        html='<div style="width:100%;padding: 5px 12px;line-height: 17px; font-size: 12px;background:rgba(0,0,0,0.03);">\
            <div><span>发件人：</span><span style="color: #0082FA;">ArtStatio</span></div>\
            <div><span>发送日期：</span><span>ArtStatio</span></div>\
            <div><span>发送至：</span><span style="color: #0082FA;">ArtStatio</span></div>\
            <div><span>主题：</span><span>ArtStatio</span></div>\
        </div>'
    }
    return html
}
function clearShowHtml(){
    //$("body").empty();
    console.log("ddddassasasa\n");
    document.body.innerHTML=''
}
//复制按钮
function copy(){
    var range = window.getSelection?window.getSelection().getRangeAt(0):document.selection.createRange().text;
    var element = $("<textarea>" + range + "</textarea>");
    $("body").append(element);
    element[0].select();
    document.execCommand("copy",false,null);
    element.remove();
}
function addContextMenuEvent(data1, data2){
    rng = window.getSelection?window.getSelection():document.selection.createRange().text;
    _editInfo = $('#editInfo');
    //复制
    if(data1['sign']=='copy'){
        document.execCommand("copy");

    }
    //粘贴
    else if(data1['sign']=='paste'){
        if(rng!=""){
             document.execCommand("delete");
        }
        connectQtObject(function() {
            window.view_bridge.onWebMessage("paste", "");
        });
    }
    //图片保存
    else if(data1['sign']=='copyimg'){
        document.execCommand("copy");
        var src = $(node).attr('src')
        connectQtObject(function() {
            window.view_bridge.onWebMessage("imgcopy", src);
        });
    }
    //复制图片
    else if(data1['sign']=='copyurl'){
        var element = $("<textarea>" + node + "</textarea>");
        $("body").append(element);
        element[0].select();
        document.execCommand("copy",false,null);
        element.remove();

    }
    //剪切
    else if(data1['sign']=='cut'){
        document.execCommand("cut");
        if(rng){
            document.execCommand("delete");
        }
    }
    //删除
    else if(data1['sign']=='delete'){
        $('#summernote').summernote('focus');
            if(rng!=""){
               document.execCommand("delete");
            }
   }else if(data1['sign']=='selectall'){
        //全选
        document.execCommand("selectAll");

   }else if(data1['sign']=='imgsave'){
        //图片另存为
        var src = $(node).attr('src')
        connectQtObject(function() {
            window.view_bridge.onWebMessage("imgsave", src);
        });

   }
    else if(data1['sign']=='newwindow'){
        //新窗口
        connectQtObject(function() {
            window.view_bridge.onWebMessage("newwindow", node.toString());
        });
    }
}
var contextMenuObjbak;

document.ondragover = function (e) { e.preventDefault(); };
document.ondrop = function (e) { e.preventDefault();};
document.ondragstart=function (e) {
    return false;
};
window.onload=function(){
    //监听编辑器change
    $('#summernote').on('summernote.change', function(we,contents, $editable) {
            connectQtObject(function() {
                window.view_bridge.onWebMessage("contentchanged", '');
            });
    });
    //编辑的页面的点击逻辑
    var rng;
    var clipboardData;
    var range;
    var contextMenuObj = null;
    var arr;
    var arr2;
    var content=$('.note-editing-area').length>0?$('.note-editing-area'):$('body');
    $(document).on('contextmenu',content,function(){
            return false;
        });
    var node;

    content.on('mousedown',function(e){
        $('#summernote').summernote('focus');
        if($(e.target).hasClass('context-li')){
            return
        }
       var  contextListParam  = [ //列表空白处右击菜单选项 disabled为是否禁用当前选项
       ];
       var selection = window.getSelection();
            if(e.which != 3){
                $('.context-menu-wrap').hide();
                return;
            }
            var tagName=e.target.tagName;//当前点击元素标签名
            node=e.target;// 当前点击的元素内容
            rng = window.getSelection?window.getSelection():document.selection.createRange().text;
            if(tagName=='A' && (node!="")){
                // 超链接标签逻辑
                var arr= [
                            {"id": 1, "name": "新窗口打开链接", "sign": "newwindow","disabled":false},
                            {"id": 2, "name": "复制链接", "sign": "copyurl","disabled":false},
                            {"id": 3, "name": "全选", "sign": "selectall","disabled":false},
                        ]
                contextListParam.push(arr);

            }else if(tagName=='IMG'){
                //图片逻辑
                arr= [
                        {"id": 1, "name": "图片另存为", "sign": "imgsave","disabled":false},
                        {"id": 2, "name": "复制图片", "sign": "copyimg","disabled":false},
                        {"id": 3, "name": "全选", "sign": "selectall","disabled":false},
                    ]
                contextListParam.push(arr);
                range = document.createRange();
                range.selectNode(node);
                selection.removeAllRanges();
                selection.addRange(range);
            }else{
                if(rng!=''){
                    //选中文字逻辑
                    arr= [
                            {"id": 1, "name": "复制", "sign": "copy","disabled":false},
                            {"id": 2, "name": "全选", "sign": "selectall","disabled":false},
                        ]
                    arr2=[
                                {"id": 1, "name": "复制", "sign": "copy","disabled":false},
                                {"id": 2, "name": "粘贴", "sign": "paste","disabled":false},
                                {"id": 3, "name": "剪切", "sign": "cut","disabled":false},
                                {"id": 4, "name": "删除", "sign": "delete","disabled":false},
                                {"id": 5, "name": "全选", "sign": "selectall","disabled":false}
                            ]
                    if($('.note-editing-area').length>0){
                        contextListParam.push(arr2);
                    }else{
                        contextListParam.push(arr);
                    }

                }else{
                    //没有选中文本逻辑
                    arr= [
                            {"id": 4, "name": "全选", "sign": "selectall","disabled":false},
                        ]
                    arr2=[
                            {"id": 1, "name": "复制", "sign": "copy","disabled":false},
                            {"id": 2, "name": "粘贴", "sign": "paste","disabled":false},
                            {"id": 3, "name": "剪切", "sign": "cut","disabled":false},
                            {"id": 4, "name": "删除", "sign": "delete","disabled":false},
                            {"id": 5, "name": "全选", "sign": "selectall","disabled":false}
                            ]
                     if($('.note-editing-area').length>0){
                          contextListParam.push(arr2);
                     }else{
                        contextListParam.push(arr);
                     }

                }
            }
            console.log($(this).data())
            contextMenuObj = $(this).contextMenu(e, contextListParam, $(this).data(), addContextMenuEvent);
    });

}
//搜索高亮20200526
//keyword 关键字 isdiff是否区分大小写
function searchKey(keyword,string,isdiff,color){
    if($("body").length>0){
        color=color?color:'#0091FF';
        $("body").textSearch(keyword,isdiff,color);
    }
}
//隐藏右键
function menuHide(){
    $('#contextMenuWrap').hide();
}
//设置a标签新窗口打开
function ulrSetBlank(){
    $('a').each(function(){
        $(this).attr('target','_Blank');
    });
}
//粘贴走qt
function fnPaste(msg,value){
    connectQtObject(function() {
        window.view_bridge.onWebMessage(msg,value);
    });
}
// 提示页面js
function setmsg(text){
    $('.main .info').html(text);
}
window.onload=function(){
    $('#summernote').summernote('change', function(we,contents, $editable) {

});
//     var contextMenuObj = null,
//     contextListParam  = [ //列表空白处右击菜单选项 disabled为是否禁用当前选项
//     [
//         {"id": 1, "name": "复制", "sign": "copy","disabled":false},
//         {"id": 1, "name": "粘贴", "sign": "paste","disabled":false}
//     ]
// ];
// $('body').contextmenu(function(){
//         return false;
//     });
    //编辑的页面的点击逻辑
    var rng;
    var clipboardData;
    var range;
    var contextMenuObj = null;
    var arr;
    var arr2;
    var int;
    var content=$('.note-editing-area').length>0?$('.note-editing-area'):$('body');
    $(document).on('contextmenu',content,function(){
            return false;
        });
    var node;
    $(document).on('mousedown','.modal-content input',function(e){
        int=$(this);
    if(e.which != 3){
        return;
    }
    var contextListParambak= []
    arr=[
        {"id": 1, "name": "复制", "sign": "copy","disabled":false},
        {"id": 2, "name": "粘贴", "sign": "paste","disabled":false},
        {"id": 3, "name": "剪切", "sign": "cut","disabled":false},
        {"id": 4, "name": "删除", "sign": "delete","disabled":false},
        {"id": 5, "name": "全选", "sign": "selectall","disabled":false}
    ]
    contextListParambak.push(arr)
    console.log(arr)
    contextMenuObj = $(this).contextMenu(e, contextListParambak, $(this).data(), addContextMenuEvent1,1);

})
function addContextMenuEvent1(data1, data2){
            //复制
        if(data1['sign']=='copy'){
            
            document.execCommand("paste");
        }
        //粘贴
        else if(data1['sign']=='paste'){
            int.focus();
            setTimeout(function() {
                document.execCommand('paste');
            },100)
            

        }
}

content.on('mousedown',function(e){
    console.log(content)
        $('#summernote').summernote('focus');
        if($(e.target).hasClass('context-li')){
            return
        }
       var  contextListParam1  = [ //列表空白处右击菜单选项 disabled为是否禁用当前选项
                                {"id": 1, "name": "复制", "sign": "copy","disabled":false},
                                {"id": 2, "name": "粘贴", "sign": "paste","disabled":false},
                                {"id": 3, "name": "剪切", "sign": "cut","disabled":false},
                                {"id": 4, "name": "删除", "sign": "delete","disabled":false},
                                {"id": 5, "name": "全选", "sign": "selectall","disabled":false}
       ];
            if(e.which != 3){
                $('.context-menu-wrap').hide();
                return;
            }
                //图片逻辑
                arr= [
                        {"id": 1, "name": "图片另存为", "sign": "imgsave","disabled":false},
                        {"id": 2, "name": "复制图片", "sign": "copyimg","disabled":false},
                        {"id": 3, "name": "全选", "sign": "selectall","disabled":false},
                    ]
                contextListParam1.push(arr);
            
            contextMenuObj = $(this).contextMenu(e, contextListParam1, $(this).data(), addContextMenuEvent);
    });
    content.on('mousedown',function(e){
        $('#summernote').summernote('focus');
        if($(e.target).hasClass('context-li')){
            return
        }
       var  contextListParam  = [ //列表空白处右击菜单选项 disabled为是否禁用当前选项
       ];
       var selection = window.getSelection();
            if(e.which != 3){
                $('.context-menu-wrap').hide();
                return;
            }
            var tagName=e.target.tagName;//当前点击元素标签名
            node=e.target;// 当前点击的元素内容
            rng = window.getSelection?window.getSelection():document.selection.createRange().text;
            if(tagName=='A' && (node!="")){
                // 超链接标签逻辑
                var arr= [
                            {"id": 1, "name": "新窗口打开链接", "sign": "newwindow","disabled":false},
                            {"id": 2, "name": "复制链接", "sign": "copyurl","disabled":false},
                            {"id": 3, "name": "全选", "sign": "selectall","disabled":false},
                        ]
                contextListParam.push(arr);

            }else if(tagName=='IMG'){
                //图片逻辑
                arr= [
                        {"id": 1, "name": "图片另存为", "sign": "imgsave","disabled":false},
                        {"id": 2, "name": "复制图片", "sign": "copyimg","disabled":false},
                        {"id": 3, "name": "全选", "sign": "selectall","disabled":false},
                    ]
                contextListParam.push(arr);
                range = document.createRange();
                range.selectNode(node);
                selection.removeAllRanges();
                selection.addRange(range);
            }else{
                if(rng!=''){
                    //选中文字逻辑
                    arr= [
                            {"id": 1, "name": "复制", "sign": "copy","disabled":false},
                            {"id": 2, "name": "全选", "sign": "selectall","disabled":false},
                        ]
                    arr2=[
                                {"id": 1, "name": "复制", "sign": "copy","disabled":false},
                                {"id": 2, "name": "粘贴", "sign": "paste","disabled":false},
                                {"id": 3, "name": "剪切", "sign": "cut","disabled":false},
                                {"id": 4, "name": "删除", "sign": "delete","disabled":false},
                                {"id": 5, "name": "全选", "sign": "selectall","disabled":false}
                            ]
                    if($('.note-editing-area').length>0){
                        contextListParam.push(arr2);
                    }else{
                        contextListParam.push(arr);
                    }

                }else{
                    //没有选中文本逻辑
                    arr= [
                            {"id": 4, "name": "全选", "sign": "selectall","disabled":false},
                        ]
                    arr2=[
                            {"id": 1, "name": "复制", "sign": "copy","disabled":false},
                            {"id": 2, "name": "粘贴", "sign": "paste","disabled":false},
                            {"id": 3, "name": "剪切", "sign": "cut","disabled":false},
                            {"id": 4, "name": "删除", "sign": "delete","disabled":false},
                            {"id": 5, "name": "全选", "sign": "selectall","disabled":false}
                            ]
                     if($('.note-editing-area').length>0){
                          contextListParam.push(arr2);
                     }else{
                        contextListParam.push(arr);
                     }

                }
            }
            console.log(this)
            contextMenuObj = $(this).contextMenu(e, contextListParam, $(this).data(), addContextMenuEvent);
    });
function _copy(anniu,content){
        var clipboard = new Clipboard(anniu, {
        target: function(e) {
            return document.querySelector(content);
        }
    });

        clipboard.on('success', function(e) {
            alert('复制成功！！');
            console.info('Action:', e.action);
            console.info('Text:', e.text);
            console.info('Trigger:', e.trigger);
            e.clearSelection();
        });

    clipboard.on('error', function(e) {
        console.error('Action:', e.action);
        console.error('Trigger:', e.trigger);
    });
}
function addContextMenuEvent(data1, data2){
        _editInfo = $('#editInfo');
        rng = window.getSelection?window.getSelection():document.selection.createRange().text;
        //复制
        if(data1['sign']=='copy'){
            document.execCommand("copy");

        }
        //粘贴
        else if(data1['sign']=='paste'){
            if(rng!=""){
                 document.execCommand("delete");
            }
            connectQtObject(function() {
                window.view_bridge.onWebMessage("paste", "");
            });
        }
        //图片保存
        else if(data1['sign']=='copyimg'){
            document.execCommand("copy");
            var src = $(node).attr('src')
            connectQtObject(function() {
                window.view_bridge.onWebMessage("imgcopy", src);
            });
        }
        //复制图片
        else if(data1['sign']=='copyurl'){
            var element = $("<textarea>" + node + "</textarea>");
            $("body").append(element);
            element[0].select();
            document.execCommand("copy",false,null);
            element.remove();

        }
        //剪切
        else if(data1['sign']=='cut'){
            document.execCommand("cut");
            if(rng){
                document.execCommand("delete");
            }
        }
        //删除
        else if(data1['sign']=='delete'){
            $('#summernote').summernote('focus');
            if(rng!=""){
                 document.execCommand("delete");
            }
       }else if(data1['sign']=='selectall'){
            //全选
            document.execCommand("selectAll");

       }else if(data1['sign']=='imgsave'){
            //图片另存为
            var src = $(node).attr('src')
            connectQtObject(function() {
                window.view_bridge.onWebMessage("imgsave", src);
            });

       }
        else if(data1['sign']=='newwindow'){
            //新窗口
            connectQtObject(function() {
                window.view_bridge.onWebMessage("newwindow", node.toString());
            });
        }
    }
}


$(document).on('click','.note-toolbar.panel-heading,.reset-btn-default',function(){
    $('.note-table-popover').hide();
})














