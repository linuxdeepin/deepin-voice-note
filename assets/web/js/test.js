function readyforEdit(op,msg,placeholderStr,language,str)
{
    var syt='发件人:\"ldz618@126.com\" <ldz618@126.com>|发件日期:2020年7月24日 下午05:42|发件至:ldz618 <ldz618@tom.com>|主题:\\';
    var nodes = document.body.childNodes;
    //console.log(document.body)
    var a=document.body;
    var summernote = document.createElement('div');
    summernote.id='summernote';
    var signDiv = document.createElement('div');
    signDiv.id = "MailBelowDiv";
    summernote.appendChild(signDiv);
    window.signDiv = signDiv;
    var slipDiv = document.createElement('div');
    slipDiv.style = "word-wrap:break-word;";
    if (op == 1 ) {
        slipDiv.innerHTML = "<br><br>";
    } else if (op > 2) {
        placeholderStr="";
        if(str && str!=''){
            slipDiv.innerHTML = "<br><br>-----------------------------" + msg +"-------------------------------------<br>"+rendhtml(op,str);
        }else{
            slipDiv.innerHTML = "<br><br>-----------------------------" + msg +"-------------------------------------<br>";
        }

    }else{
        placeholderStr="";
    }
    summernote.appendChild(slipDiv);
    //summernote.appendChild(a)
    //document.body.replaceChild(summernote,document.body)
    //console.log(nodes)
    //console.log(summernote)
   // NodeList.prototype.forEach = Array.prototype.forEach;
    //summernote.append(document.body.innerHTML)
    var arr=[];
    nodes.forEach(function (node,index) {
        console.log(node)
        arr.push(node)
        //node.parentNode.replaceChild(summernote, node)
        //summernote.appendChild(node)
     })
     console.log(arr)
     arr.forEach(function(node,index){
        node.parentNode.replaceChild(summernote, node)
        summernote.appendChild(node)
     })
    // for(var i=0;i<nodes.length;i++){
    //     if(nodes[i].nodeType==1){
    //         nodes[i].parentNode.replaceChild(summernote, nodes[i]);
    //         summernote.appendChild(nodes[i])
    //     } 
    // }
    // for(var i in nodes){
    //     console.log(nodes[i])
    //     if(nodes[i]){
    //         //nodes[i].parentNode.replaceChild(summernote, nodes[i]);
    //         //summernote.appendChild(nodes[i])
    //     } 
    // }
    var Mgb=document.getElementsByTagName('meta');
    Mgb.content="text/html; charset=utf-8";
    console.log(Mgb)


    var script_item = document.createElement("script");
    var type=language.indexOf('zh')>-1?'zh-CN':'';
    script_item.type="text/javascript";
    script_item.innerHTML="    \
            $('#summernote').summernote({ \
                lang: \'"+ type +"\', \
                height:null,\
                minHeight: null, \
                maxHeight: null,  \
                placeholder: \'"+ placeholderStr +"\',\
                toolbar: [\
                        ['fontname', ['fontname']],\
                        ['fontsize', ['fontsize']],\
                        ['forecolor', ['forecolor']],\
                        ['forecolor', ['backcolor']],\
                        ['style', ['bold', 'italic', 'underline', 'strikethrough']],\
                        ['para', ['justifyLeft', 'justifyCenter', 'justifyRight','justifyFull']],\
                        ['para', ['indent', 'outdent']],\
                        ['table', ['table']],\
                        ['para', ['ul','ol']],\
                        ['undo',['undo']], \
                        ['redo',['redo']],\
                        ['link', ['link',]]\
                   ],\
              });\
            $('#summernote').summernote('fullscreen.toggle');";
            return
    document.body.appendChild(script_item);
}
/*增加转发提示*/
function rendhtml(op,str){
    var html='';
    var arr=str.split(',')
    var style="color:#0082FA"
    if(op==5){
        for(var i=0;i<arr.length;i++){
            html+='<div><span>'+arr[i].split(':')[0]+'：</span><span style='+(i%2==0?style:"")+'>'+arr[i].substr(arr[i].indexOf(':')+1)+'</span></div>';
        };
        return '<div style="width:100%;padding: 5px 12px;line-height: 17px; font-size: 12px;background:rgba(0,0,0,0.03);">'+html+'</div>';
    }else{
        return html
    }
}