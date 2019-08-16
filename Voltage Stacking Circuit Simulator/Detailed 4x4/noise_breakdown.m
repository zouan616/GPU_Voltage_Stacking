data1=importdata('hotspot_power.txt');
data2=importdata('hotspot_2level_power.txt');
x1 = data1(100000:1:110000,1);
x2 = data1(100000:1:110000,2);
x3 = data1(100000:1:110000,3);
x4 = data1(100000:1:110000,4);
x5 = data1(100000:1:110000,5);
x6 = data1(100000:1:110000,6);
x7 = data1(100000:1:110000,7);
x8 = data1(100000:1:110000,8);
x9 = data1(100000:1:110000,9);
x10 = data1(100000:1:110000,10);
x11 = data1(100000:1:110000,11);
x12 = data1(100000:1:110000,12);
x13 = data1(100000:1:110000,13);
x14 = data1(100000:1:110000,14);
x15 = data1(100000:1:110000,15);

I_g = (x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10 + x11 + x12 + x13 + x14 + x15)./15;

I_st_1 = (x1 + x6 + x11)./3  - I_g;
I_st_2 = (x2 + x7 + x12)./3  - I_g;
I_st_3 = (x3 + x8 + x13)./3  - I_g;
I_st_4 = (x4 + x9 + x14)./3  - I_g;
I_st_5 = (x5 + x10 + x15)./3  - I_g;


I_r_1 = (2*x1 - x6 - x11)./3;
I_r_2 = (2*x2 - x7 - x12)./3;
I_r_3 = (2*x3 - x8 - x13)./3;
I_r_4 = (2*x4 - x9 - x14)./3;
I_r_5 = (2*x5 - x10 - x15)./3;

I_r_6 = (2*x6 - x1 - x11)./3;
I_r_7 = (2*x7 - x2 - x12)./3;
I_r_8 = (2*x8 - x3 - x13)./3;
I_r_9 = (2*x9 - x4 - x14)./3;
I_r_10 = (2*x10 - x5 - x15)./3;

I_r_11 = (2*x11 - x1 - x6)./3;
I_r_12 = (2*x12 - x2 - x7)./3;
I_r_13 = (2*x13 - x3 - x8)./3;
I_r_14 = (2*x14 - x4 - x9)./3;
I_r_15 = (2*x15 - x5 - x10)./3;

x = -6:0.5:6;

x21 = data2(100000:1:110000,1);
x22 = data2(100000:1:110000,2);
x23 = data2(100000:1:110000,3);
x24 = data2(100000:1:110000,4);
x25 = data2(100000:1:110000,5);
x26 = data2(100000:1:110000,6);
x27 = data2(100000:1:110000,7);
x28 = data2(100000:1:110000,8);
x29 = data2(100000:1:110000,9);
x210 = data2(100000:1:110000,10);
x211 = data2(100000:1:110000,11);
x212 = data2(100000:1:110000,12);
x213 = data2(100000:1:110000,13);
x214 = data2(100000:1:110000,14);
x215 = data2(100000:1:110000,15);

I2_g = (x21 + x22 + x23 + x24 + x25 + x26 + x27 + x28 + x29 + x210 + x211 + x212 + x213 + x214 + x215)./215;

I2_st_1 = (x21 + x26 + x211)./3  - I2_g;
I2_st_2 = (x22 + x27 + x212)./3  - I2_g;
I2_st_3 = (x23 + x28 + x213)./3  - I2_g;
I2_st_4 = (x24 + x29 + x214)./3  - I2_g;
I2_st_5 = (x25 + x210 + x215)./3  - I2_g;


I2_r_1 = (2*x21 - x26 - x211)./3;
I2_r_2 = (2*x22 - x27 - x212)./3;
I2_r_3 = (2*x23 - x28 - x213)./3;
I2_r_4 = (2*x24 - x29 - x214)./3;
I2_r_5 = (2*x25 - x210 - x215)./3;

I2_r_6 = (2*x26 - x21 - x211)./3;
I2_r_7 = (2*x27 - x22 - x212)./3;
I2_r_8 = (2*x28 - x23 - x213)./3;
I2_r_9 = (2*x29 - x24 - x214)./3;
I2_r_10 = (2*x210 - x25 - x215)./3;

I2_r_11 = (2*x211 - x21 - x26)./3;
I2_r_12 = (2*x212 - x22 - x27)./3;
I2_r_13 = (2*x213 - x23 - x28)./3;
I2_r_14 = (2*x214 - x24 - x29)./3;
I2_r_15 = (2*x215 - x25 - x210)./3;


histogram(I_r_1,x, 'FaceColor',[194 24 24]./255, 'EdgeColor', 'k','LineWidth',0.5,'FaceAlpha',0.5)
hold on
histogram(I2_r_1,x, 'FaceAlpha',0.5, 'EdgeColor', 'k', 'LineWidth',0.5,'FaceColor',[8 108 162]./255)
legend('No Architecture Assistance','VS')

hold on

set(gca,'FontSize',20);

grid on

axis([-7 7 0 1500]);