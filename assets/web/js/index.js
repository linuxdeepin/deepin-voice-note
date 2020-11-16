function readyforEdit(op,msg,placeholderStr,language)
{
    var isTrue=document.getElementById('summernote');
    console.log(isTrue)
    if(!$('#summernote').length>0){
    var nodes = document.body.childNodes;
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
        slipDiv.innerHTML = "<br><br>-----------------------------" + msg +"-------------------------------------<br>";
    }
    summernote.appendChild(slipDiv);

    nodes.forEach(function (node,index) {
         node.parentNode.replaceChild(summernote, node)
         summernote.appendChild(node)
     })

    var script_item = document.createElement("script");
    var type=language?'zh-CN':'';
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
            $('#summernote').summernote('fullscreen.toggle');"
    document.body.appendChild(script_item);
}
}