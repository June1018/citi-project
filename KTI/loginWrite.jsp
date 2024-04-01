
<%@ page language="java" contentType="text/html; charset=UTF-8"
    pageEncoding="UTF-8"%>
    
<!DOCTYPE html>
<html>
<head>
<style>
body{
	font-size:9pt;
	font-color:#333333;
	font-family:맑은고딕;
}

a{
	text-decoration:none;
}
table{
	width:600px;
	border-collapse:collapse;
}
th, td{
	border:1px solid #cccccc;
	padding:3px;
	line-height:2;  
}
caption{
    font-size:15pt;
    font-weight:bold;
    margin-top:10px;
    padding-bottom:5px;
}
.div_button{
	width:600px;
	text-align:center;
	margin-top:5px;
}
<!-- line-height:2 주소값 사이 간격(2)-->
</style>
<meta charset="UTF-8">
<title>회원로그인화면</title>

<script src="/myproject_new/script/jquery-1.12.4.js"></script>  <!-- jquery문법 -->


 <script>
$( function() {
	 
   $("#btn_submit").click(function(){
	   var userid = $.trim($("#userid").val());
	   var pass   = $.trim($("#pass").val());

	   if( userid == ""){
	       alert("아이디를 입력해주세요.");
		   $("#userid").focus();
		   return false;
       }
	   if( pass == ""){
	       alert("암호을 입력해주세요.");
	       $("#pass").focus();
		   return false;
       }
	   var formData = $("#frm").serialize();  /* form 아래의 데이터   */
	   alert("formData"+$("#frm").serialize());
	   
	   //ajax : 비동기 전송방식의 기능을 가지고 있는 jquery의 함수
	   $.ajax({
		   type:"POST",
		   data:formData,    //json타입 전송
		   url:"loginWriteSub.do",
		   dataType:"text",                      // 리턴타입
		   
		   /* 전송후 셋팅 */
		   success:function(result){             //controller -> result ="ok"셋팅 
		   
		       if(result == "ok"){               // ajax : result int리턴값으로 하면 안된다. String 타입으로 해야함 
				   alert(userid+"님은 로그인 되었습니다.");
		           location="boardList.do";
			   }else{                            //장애 발생
				   alert("로그인 정보를 다시 확인해주세요.");
			   }
			},
			error:function(){
		       alert("오류발생");
			}		
		});	   
   }); 
   
});
 </script>
</head>
<body>
<%@ include file="../include/topmenu.jsp" %>
<form name="frm" id="frm">
<table>
    <caption>로그인</caption>
    <tr>
    	<th><label for="userid">아이디</label></th>
    	<td>
    	<input type="text" name="userid" id="userid" placeholder="아이디입력">  <!-- 초기출력시 아이디입력 나옴 -->
    	</td>
    </tr>
    <tr>
    	<th><label for="pass">암호</label></th>
    	<td>
    	<input type="password" name="pass" id="pass">
    	</td>
    </tr>
</table>
<div class="div_button">
 	<button type="button" id="btn_submit">로그인</button>
 	<button type="reset" id="btn_reset">취소</button>
</div>
</form>
 
</body>
</html>