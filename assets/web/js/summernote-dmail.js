/****************************************************************************
*/
    $(document).ready(function() {
        $('#summernote').summernote({
             lang: 'zh-CN',
             placeholder: '请在此输入内容...',
             height: 300,
             width:800,
             callbacks: {
                  onInit: function() {
                      //init
                  },
                  onFocus: function() {
                      //focus
                  },
                  onImageUpload: function(files, editor, $editable) {
                      data = new FormData();  
                      data.append("file", files[0]);  
                  }
                }
             
        });
    });

