# AD-Census
AD-Census立体匹配算法，中国学者Xing Mei等人研究成果（Respect！），算法效率高、效果出色，适合硬件加速，Intel RealSense D400 Stereo模块算法。完整实现，代码规范，注释清晰，欢迎star！

<table>
    <tr>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/Data/Cone/im2.png"></center></td>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/Data/Cone/im6.png"></center></td>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/doc/exp/res/cone-d.png"></center></td>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/doc/exp/res/cone-c.png"></center></td>
    </tr>
    <tr>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/Data/Cloth3/view1.png"></center></td>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/Data/Cloth3/view5.png"></center></td>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/doc/exp/res/cloth-d.png"></center></td>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/doc/exp/res/cloth-c.png"></center></td>
    </tr>
    <tr>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/Data/Piano/im0.png"></center></td>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/Data/Piano/im1.png"></center></td>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/doc/exp/res/piano-d.png"></center></td>
        <td ><center><img src="https://github.com/ethan-li-coding/AD-Census/blob/master/doc/exp/res/piano-c.png"></center></td>
    </tr>
<table>

# 环境
windows10 / visual studio 2015&2019
<br>代码基本没有使用系统api，你可以非常方便的移植到linux，可能需要做极少量的修改

# 第三方库
opencv310
<br>
百度网盘连接：https://pan.baidu.com/s/1_WD-KdPyDBazEIim7NU3jA 
<br>
提取码：aab4
<br><br>
解压后放将名称为OpenCV的文件夹复制到到3rdparty文件夹下
<br><br>若运行时提示缺少opencv_310(d).dll，则在OpenCV文件夹里找到对应的dll文件复制到程序exe所在的目录即可（Opencv\dll\opencv_310(d).dll），带d为debug库，不带d为release库。
<br><br>
为便于移植，算法是不依赖任何图像库的，只在算法实验部分调用opencv库读取和显示图像，也可替换成其他图像库

## 论文
Mei X , Sun X , Zhou M , et al. <b>On building an accurate stereo matching system on graphics hardware</b>[C]// IEEE International Conference on Computer Vision Workshops. IEEE, 2012.

## Github图片不显示的解决办法
修改hosts

C:\Windows\System32\drivers\etc\hosts

在文件末尾添加：

``` cpp
# GitHub Start 
192.30.253.112    github.com 
192.30.253.119    gist.github.com
151.101.184.133    assets-cdn.github.com
151.101.184.133    raw.githubusercontent.com
151.101.184.133    gist.githubusercontent.com
151.101.184.133    cloud.githubusercontent.com
151.101.184.133    camo.githubusercontent.com
151.101.184.133    avatars0.githubusercontent.com
151.101.184.133    avatars1.githubusercontent.com
151.101.184.133    avatars2.githubusercontent.com
151.101.184.133    avatars3.githubusercontent.com
151.101.184.133    avatars4.githubusercontent.com
151.101.184.133    avatars5.githubusercontent.com
151.101.184.133    avatars6.githubusercontent.com
151.101.184.133    avatars7.githubusercontent.com
151.101.184.133    avatars8.githubusercontent.com
 
 # GitHub End
```
