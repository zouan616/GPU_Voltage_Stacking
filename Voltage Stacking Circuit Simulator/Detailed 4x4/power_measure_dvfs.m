clear
data1=importdata('voltage_PDE.txt');
data2=importdata('power_PDE.txt');
x1 = data1(3:1:12,1);
y1 = data2(1000:1:6000,1);

x2 = data1(3:1:12,2);
y2 = data2(1000:1:6000,2);

x3 = data1(3:1:12,3);
y3 = data2(1000:1:6000,3);

x4 = data1(3:1:12,4);
y4 = data2(1000:1:6000,4);

x5 = data1(3:1:12,5);
y5 = data2(1000:1:6000,5);

x6 = data1(3:1:12,6);
y6 = data2(1000:1:6000,6)

x7 = data1(3:1:12,7);
y7 = data2(1000:1:6000,7);

x8 = data1(3:1:12,8);
y8 = data2(1000:1:6000,8);

x9 = data1(3:1:12,9);
y9 = data2(1000:1:6000,9);

x10 = data1(3:1:12,10);
y10 = data2(1000:1:6000,10);

x11 = data1(3:1:12,11);
y11 = data2(1000:1:6000,11);

x12 = data1(3:1:12,12);
y12 = data2(1000:1:6000,12);

x13 = data1(3:1:12,13);
y13 = data2(1000:1:6000,13);

x14 = data1(3:1:12,14);
y14 = data2(1000:1:6000,14);

x15 = data1(3:1:12,15);
y15 = data2(1000:1:6000,15);

x16 = data1(3:1:12,16);
y16 = data2(1000:1:6000,16);

power1 = mean(x1)*mean(y1) + mean(x1)*3;
power2 = mean(x2)*mean(y2) + mean(x2)*3;
power3 = mean(x3)*mean(y3) + mean(x3)*3;
power4 = mean(x4)*mean(y4) + mean(x4)*3;
power5 = mean(x5)*mean(y5) + mean(x5)*3;
power6 = mean(x6)*mean(y6) + mean(x6)*3;
power7 = mean(x7)*mean(y7) + mean(x7)*3;
power8 = mean(x8)*mean(y8) + mean(x8)*3;
power9 = mean(x9)*mean(y9) + mean(x9)*3;
power10 = mean(x10)*mean(y10) + mean(x10)*3;
power11 = mean(x11)*mean(y11) + mean(x11)*3;
power12 = mean(x12)*mean(y12) + mean(x12)*3;
power13 = mean(x13)*mean(y13) + mean(x13)*3;
power14 = mean(x14)*mean(y14) + mean(x14)*3;
power15 = mean(x15)*mean(y15) + mean(x15)*3;
power16 = mean(x16)*mean(y16) + mean(x16)*3;



power = mean(power1 + power2 + power3 + power4 + power5 + power6 + power7 + power8 + power9 + power10 + power11 + power12 + power13 + power14 + power15 + power16)

