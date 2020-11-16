// by zhangxixnu 2010-06-21  welcome to visit my personal website http://www.zhangxinxu.com/
// textSearch.js v1.0 文字，关键字的页面纯客户端搜索
// 2010-06-23 修复多字母检索标签破碎的问题
// 2010-06-29 修复页面注释显示的问题
// 2013-05-07 修复继续搜素关键字包含之前搜索关键字没有结果的问题
// 不论何种情况，务必保留作者署名。 


(function($){
	$.fn.textSearch = function(str,isdiff,options){
		var defaults = {
			divFlag: true,
			divStr: " ",
			markClass: "",
			markColor: "red",
			nullReport: true,
			callback: function(){
				return false;	
			}
		};
		var sets = $.extend({}, defaults, options || {}), clStr;
		if(sets.markClass){
			clStr = "class='"+sets.markClass+"'";	
		}else{
			clStr = "style='color:"+sets.markColor+";'";
		}
		
		//对前一次高亮处理的文字还原		
		$("span[rel='mark']").each(function() {
			var text = document.createTextNode($(this).text());	
			$(this).replaceWith($(text));
		});
		
		
		//字符串正则表达式关键字转化
		$.regTrim = function(s){
			var imp = /[\^\.\\\|\(\)\*\+\-\$\[\]\?]/g;
			var imp_c = {};
			imp_c["^"] = "\\^";
			imp_c["."] = "\\.";
			imp_c["\\"] = "\\\\";
			imp_c["|"] = "\\|";
			imp_c["("] = "\\(";
			imp_c[")"] = "\\)";
			imp_c["*"] = "\\*";
			imp_c["+"] = "\\+";
			imp_c["-"] = "\\-";
			imp_c["$"] = "\$";
			imp_c["["] = "\\[";
			imp_c["]"] = "\\]";
			imp_c["?"] = "\\?";
			s = s.replace(imp,function(o){
				return imp_c[o];					   
			});	
			return s;
		};
		$(this).each(function(){
			var t = $(this);
			str = $.trim(str);
			if(str === ""){
				return false;
			}else{
				//将关键字push到数组之中
				var arr = [];
				if(sets.divFlag){
					arr = str.split(sets.divStr);	
				}else{
					arr.push(str);	
				}
			}
			var v_html = t.html();
			
			//删除注释
			v_html = v_html.replace(/<!--(?:.*)\-->/g,"");
			
			//将HTML代码支离为HTML片段和文字片段，其中文字片段用于正则替换处理，而HTML片段置之不理
			var tags = /[^<>]+|<(\/?)([A-Za-z]+)([^<>]*)>/g;
			var a = v_html.match(tags), test = 0;
			var regE="";
			if(!isdiff && /[a-zA-Z]/.test(str)){
				// var reg = new RegExp($.regTrim(str), "gi");
				// v_html = v_html.replace(reg, function(txt){
				// 	return "<span rel='mark' "+clStr+">"+txt+"</span>";
				// })
				$.each(a, function(i, c){
					if(!/<(?:.|\s)*?>/.test(c)){//非标签
						console.log(c)
						//开始执行替换
						//c = c.toLowerCase();
							//con = con.toLowerCase();
							if(c === ""){return;}
							var reg = new RegExp($.regTrim(str), "gi");
							var arr=['&nbsp;','&amp;']
							// c = c.replace(reg, function(txt){
							// 	return "<span rel='mark' "+clStr+">"+txt+"</span>";
							// })
							//a[i] = c;
							//优化
							if(c.indexOf('&nbsp;')>0 || c.indexOf('&amp;')>0){
								var reg1=new RegExp($.regTrim('&nbsp;'), "gi");
								c = c.replace(reg1, '==');

								var re1=new RegExp($.regTrim('&amp;'), "gi");
								c = c.replace(re1, '&&');

								c = c.replace(reg, function(txt){
									return "<span rel='mark' "+clStr+">"+txt+"</span>";
								})

								var re2=new RegExp($.regTrim('=='), "gi");
								c = c.replace(re2, '&nbsp;')

								var re2=new RegExp($.regTrim('&&'), "gi");
								c = c.replace(re2, '&amp;')
							}else{
								c = c.replace(reg, function(txt){
									return "<span rel='mark' "+clStr+">"+txt+"</span>";
								})
							}
							a[i] = c
					}
				});
				var new_html = a.join("");
				$(this).html(new_html);
			}else{
				$.each(a, function(i, c){
					if(!/<(?:.|\s)*?>/.test(c)){//非标签
						//开始执行替换
						//c = c.toLowerCase();
						$.each(arr,function(index, con){
							if(con === ""){return;}
							var reg = new RegExp($.regTrim(con), "g");
							if(reg.test(c)){
								//正则替换
								c = c.replace(reg,"♂"+con+"♀");
								test = 1;
							}
						});
						c = c.replace(/♂/g,"<span rel='mark' "+clStr+">").replace(/♀/g,"</span>");
						a[i] = c;
					}
				});
				//将支离数组重新组成字符串
				var new_html = a.join("");
				$(this).html(new_html);
			}
			if(test === 0 && sets.nullReport){
				//alert("没有搜索结果");	
				return false;
			}
			
			//执行回调函数
			sets.callback();
		});
	};
})(jQuery);