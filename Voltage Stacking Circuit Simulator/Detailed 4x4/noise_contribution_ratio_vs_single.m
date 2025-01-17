N = 6;
C = linspecer(N);
I_global = [0.09 0.09 0.04 0.04 0.06 0.06 0.07 0.07 0.06 0.06 0.04 0.04 0.06 0.06 0.05 0.05];
I_stack = [0.07 0.069 0.1013 0.1015 0.055 0.054 0.06 0.061 0.06 0.058 0.04 0.043 0.05 0.051 0.04 0.042]
I_res = [1 0 0.85 0 0.41 0 0.6 0 0.5 0 0.28 0 0.55 0 0.58 0];

I_total = I_global + I_stack + I_res;

I_global = I_global./I_total;
I_stack = I_stack./I_total;
I_res = I_res./I_total;



y = [I_stack' I_global' I_res'];
b = bar(y,0.7,'stacked');

b(1).FaceColor = [89 89 89]/255
b(2).FaceColor = [254 61 67]/255
b(3).FaceColor = [0 114 189]/255


legend('V_{stack}','V_{global}','V_{residual}','Location','NorthWest');
%_______________________________________________________________________%




set(gca, 'fontname', 'Arial');
set(gca, 'fontsize', 16);
set(gca, 'linewidth',1);
%set(gca, 'FontWeight','bold')

%stop using science count
y_formatstring = '%3.0f'; 
axis([0.5 16.5 0 1])

%xl = xlabel('\bfGTX480 Power');
yl = ylabel({'Ratio'});
%set(xl,'fontsize',20);
set(yl,'fontsize',20,'Rotation',90);
%set(xl, 'fontname', 'Arial');
set(yl, 'fontname', 'Arial');

set(gca,'xticklabel',{'backprop','hotspot','lavaMD','pathfinder','streamcluster','kmeans','MGST','leukocyte'})
h = gca;
th=rotateticklabel(h, 15);

%set(yl,'Position',get(yl,'Position') + [0 -1 0]);
%set(gca,'yticklabel',{'0','10%','20%','30%','40%','50%','60%','70%','80%','90%','100%'},'fontsize',18);









