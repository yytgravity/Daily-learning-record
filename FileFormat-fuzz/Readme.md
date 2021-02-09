记录一下学习文件格式的过程

1、初始winafl--通过复线该文章内容学习了winafl的使用 （https://www.freebuf.com/articles/system/216437.html ）

- 练习笔记：https://github.com/yytgravity/Daily-learning-record/tree/master/FileFormat-fuzz/winafl-exercise

2、通过学习DynamoRIO来初步理解插桩

- 笔记：https://github.com/yytgravity/Daily-learning-record/tree/master/FileFormat-fuzz/DynamoRIO

3、阅读了一些winafl的入门案例
- https://symeonp.github.io/2017/09/17/fuzzing-winafl.html

- https://www.apriorit.com/dev-blog/644-reverse-vulnerabilities-software-no-code-dynamic-fuzzing

4、通过（https://www.apriorit.com/dev-blog/640-qa-fuzzing-for-closed-source-windows-software ）学习如何对闭源二进制文件进行文件格式fuzz，总结如下：
- 找到我们想要fuzz的功能。
- 逆向。
- 编写一个调用上面逆向的API的程序（harness）。
- 借助DynamoRIO查看覆盖率。
- 重复上述过程，直到完全覆盖到了想要的fuzz的函数。


5、学习adobe中的文件格式fuzz

- 通过（https://research.checkpoint.com/2018/50-adobe-cves-in-50-days/ ）练习通过逆向调试的方法为JP2KLib.dll编写harness
- Adobe Reader调试符号相关（https://www.4hou.com/posts/4Y0k ）

6、gdi实战
学习资料：
- https://www.ixiacom.com/company/blog/investigating-windows-graphics-vulnerabilities-reverse-engineering-and-fuzzing-story

- https://www.pentestpartners.com/security-blog/time-travel-debugging-finding-windows-gdi-flaws/

我的gdi harness代码：
https://github.com/yytgravity/Daily-learning-record/tree/master/FileFormat-fuzz/gdi

7、pbk实战
学习资料：
- https://symeonp.github.io/2020/12/08/phonebook-uaf-analysis.html

我的harness代码：
https://github.com/yytgravity/Daily-learning-record/tree/master/FileFormat-fuzz/pbk

8、学习.LNK格式fuzz寻找攻击面

- https://blog.vincss.net/2020/06/cve49-microsoft-windows-lnk-remote-code-execution-vuln-cve-2020-1299-eng.html 
- https://www.zerodayinitiative.com/blog/2020/3/25/cve-2020-0729-remote-code-execution-through-lnk-files

我的想法（未实践）：之前大部分的关于.lnk的fuzz都是在shell32.dll和windows.storage.dll上进行的，windows.storage.search.dll 和StructuredQuery.dll应该还是有可挖掘的余地。

9、学习.CAB格式fuzz

- https://www.zerodayinitiative.com/blog/2020/7/8/cve-2020-1300-remote-code-execution-through-microsoft-windows-cab-files