(this.webpackJsonphw2=this.webpackJsonphw2||[]).push([[0],{12:function(e,t,a){},14:function(e,t,a){"use strict";a.r(t);var n=a(1),r=a(6),c=a.n(r),i=(a(12),a(2)),s=(a(5),a(3)),o=a(7),l=a(0);var j=function(e){var t=e.list.map((function(e){return Object(l.jsxs)("li",{children:[e[0],":",e[1]]})}));return Object(l.jsxs)("div",{children:[Object(l.jsx)("h4",{children:"Leaders and all other players with scores, ranking from high to low:"}),Object(l.jsx)("ul",{children:t})]})};var b=function(e){var t=e.name,a=e.currentScore,n=e.setLogin,r=e.setDeleted;(null==localStorage.getItem(t)||localStorage.getItem(t)<a)&&localStorage.setItem(t,a);var c=[],i=Object(o.a)({},localStorage);return Object.entries(i).forEach((function(e){return c.push([e[0],e[1]])})),c.sort((function(e,t){return t[1]-e[1]})),Object(l.jsxs)("div",{children:[Object(l.jsxs)("h4",{children:["Your Account:",t,Object(l.jsx)("br",{}),"Your Current Score:",a,Object(l.jsx)("br",{}),"Your Best Score:",localStorage.getItem(t),Object(l.jsx)("br",{}),"Best Among All Players:",c[0][1]]}),Object(l.jsx)("button",{type:"button",onClick:function(){localStorage.removeItem(t),r(!0),n(!1)},children:"Delete Account"}),Object(l.jsx)(j,{list:c})]})};var u=function(e){var t=e.name,a=e.setLogin,r=e.setDeleted,c=Object(n.useState)(0),o=Object(i.a)(c,2),j=o[0],u=o[1],m=Object(n.useState)(0),g=Object(i.a)(m,2),d=g[0],O=g[1],h=[1,2,3,4,5,6,7,8,9,10].sort((function(){return Math.random()-.5}));function x(){u(j+1)}return Object(l.jsx)("div",{children:j<10?Object(l.jsxs)("div",{children:[Object(l.jsx)("img",{src:s["".concat(h[j])].image,className:"image",alt:"Celebrity"}),Object(l.jsx)("br",{}),Object(l.jsx)("button",{type:"button",onClick:function(){u(j+1),O(d+1)},children:s["".concat(h[j])].A}),Object(l.jsx)("br",{}),Object(l.jsx)("button",{type:"button",onClick:x,children:s["".concat(h[j])].B}),Object(l.jsx)("br",{}),Object(l.jsx)("button",{type:"button",onClick:x,children:s["".concat(h[j])].C}),Object(l.jsx)("br",{}),Object(l.jsx)("button",{type:"button",onClick:x,children:s["".concat(h[j])].D}),Object(l.jsxs)("h2",{children:["Your Current Score:",d,"/",j]})]}):Object(l.jsx)(b,{name:t,currentScore:d,setLogin:a,setDeleted:r})})};var m=function(){var e=Object(n.useState)(""),t=Object(i.a)(e,2),a=t[0],r=t[1],c=Object(n.useState)(!1),s=Object(i.a)(c,2),o=s[0],j=s[1],b=Object(n.useState)(!0),m=Object(i.a)(b,2),g=m[0],d=m[1],O=Object(n.useState)(!1),h=Object(i.a)(O,2),x=h[0],S=h[1],C=function(){a.match(/^[a-z0-9]+$/i)?(j(!0),S(!1)):(d(!1),r(""))};return Object(l.jsxs)("div",{children:[Object(l.jsx)("h1",{children:"Guess The Celebrity"}),o?Object(l.jsx)("div",{children:Object(l.jsx)(u,{name:a,setLogin:j,setDeleted:S})}):Object(l.jsxs)("form",{onSubmit:C,children:[Object(l.jsx)("h4",{children:"Enter Your Name Here"}),Object(l.jsx)("input",{type:"text",placeholder:"username",value:a,onChange:function(e){r(e.target.value)}}),Object(l.jsx)("input",{type:"submit",name:"Start Quiz",onClick:C})]}),!g&&Object(l.jsx)("h4",{children:"Invalid User Name!!"}),x&&Object(l.jsx)("h4",{children:"Your account was successfully deleted!"})]})};c.a.render(Object(l.jsx)(m,{}),document.getElementById("root"))},3:function(e){e.exports=JSON.parse('{"1":{"image":"./images/1.jpg","A":"Harry Styles","B":"Ki Hong Lee","C":"Thomas Sangster","D":"Dylan O\'Brien"},"2":{"image":"./images/2.jpg","A":"Camila Cabello","B":"Ariana Grande","C":"Emma Stone","D":"Selena Gomez"},"3":{"image":"./images/3.jpg","A":"Selena Gomez","B":"Mackenzie Ziegler","C":"Ariana Grande","D":"Sia"},"4":{"image":"./images/4.jpg","A":"Sia","B":"Ariana Grande","C":"Taylor Swift","D":"Emma Stone"},"5":{"image":"./images/5.jpg","A":"Ariana Grande","B":"Camila Cabello","C":"Sia","D":"Maddie Ziegler"},"6":{"image":"./images/6.jpg","A":"Maddie Ziegler","B":"Camila Cabello","C":"Mackenzie Ziegler","D":"Ariana Grande"},"7":{"image":"./images/7.jpg","A":"Thomas Sangster","B":"Big Sean","C":"Jace Norman","D":"Justin Bieber"},"8":{"image":"./images/8.jpg","A":"Isabella Moner","B":"Laura Morano","C":"Kaya Scodelario","D":"Camila Cabello"},"9":{"image":"./images/9.jpg","A":"Justin Bieber","B":"Dylan O\'Brien","C":"Thomas Sangster","D":"Harry Styles"},"10":{"image":"./images/10.jpg","A":"Kaya Scodelario","B":"Ariana Grande","C":"Taylor Swift","D":"Camila Cabello"}}')},5:function(e,t,a){}},[[14,1,2]]]);
//# sourceMappingURL=main.fce699aa.chunk.js.map