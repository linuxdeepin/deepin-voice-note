$(document).ready(function(){
    $('#summernote').summernote({
        height: 300,                 // set editor height
        minHeight: null,             // set minimum height of editor
        maxHeight: null,             // set maximum height of editor
        focus: true                  // set focus to editable area after initializin
    });
    
    //点击变色
    $('body').on('click', '.li', function (e) {
        console.log('div click...');
        e.stopPropagation();
        // $(this).addClass('active').siblings('.li').removeClass('active');
        $('.li').removeClass('active');
        $(this).addClass('active');
    })
  });

  var BlockId = 0;
  var createTime = '02.20';
  var title = '语音';

  function btnClick(){
      console.log('btn click...');
    this.BlockId ++;
    var titleTmp = this.title + this.BlockId; 
    // var oHML=`
    //         <div class="li" data-id="{{this.BlockId}}">
    //                 <div class="demo" data-id={{this.BlockId}}>
    //                     <div class="left">
    //                         <div class="btn play" data-id="{{this.BlockId}}"></div>
    //                     </div>
    //                     <div class="right">
    //                         <div class="lf">
    //                             <div class="title">${titleTmp}</div>
    //                             <div class="minute padtop">${this.createTime}</div>
    //                         </div>
    //                         <div class="lr">
    //                             <div class="icon">
    //                             <div class="wifi-symbol">
    //                                 <div class="wifi-circle first"></div>
    //                                 <div class="wifi-circle second"></div>
    //                                 <div class="wifi-circle third"></div>
    //                                 <div class="wifi-circle four"></div>
    //                             </div>
    //                             </div>
    //                             <div class="time padtop">{{this.voiceSize}}</div>
    //                     </div>
    //                 </div>
    //             </div>`;
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





