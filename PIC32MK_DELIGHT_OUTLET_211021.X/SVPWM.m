%% Space Vector Modulation
clear all
close all

enc_res = 5760;
pole_num = 7;
enc_pole = enc_res/pole_num;
enc = 0:enc_res*1;
duty_resolution = 3000;
Vref = 1;
modulation_index = 0.5;

PDC1_Arr = zeros(length(enc), 1);
PDC2_Arr = zeros(length(enc), 1);
PDC3_Arr = zeros(length(enc), 1);

for i=1:length(enc)
    v_angle = mod(mod(i,enc_res)*360/enc_pole, 360);
    sector_num = floor(v_angle/60)+1;
    T1 = modulation_index*sqrt(3)*duty_resolution*Vref*sin(pi/180*(60*sector_num-v_angle));
    T2 = modulation_index*sqrt(3)*duty_resolution*Vref*sin(pi/180*(v_angle - 60*(sector_num-1)));
    T0 = duty_resolution-T1-T2;
    
    PDC1 = 0;
    PDC2 = 0;
    PDC3 = 0;
    switch sector_num
        case 1
            PDC1 = T1+T2+T0/2;
            PDC2 = T2+T0/2;
            PDC3 = T0/2;
        case 2
            PDC1 = T1+T0/2;
            PDC2 = T1+T2+T0/2;
            PDC3 = T0/2;
        case 3
            PDC1 = T0/2;
            PDC2 = T1+T2+T0/2;
            PDC3 = T2+T0/2;
        case 4
            PDC1 = T0/2;
            PDC2 = T1+T0/2;
            PDC3 = T1+T2+T0/2;
        case 5
            PDC1 = T2+T0/2;
            PDC2 = T0/2;
            PDC3 = T1+T2+T0/2;
        case 6
            PDC1 = T1+T2+T0/2;
            PDC2 = T0/2;
            PDC3 = T1+T0/2;
    end
    PDC1_Arr(i,1) = PDC1;
    PDC2_Arr(i,1) = PDC2;
    PDC3_Arr(i,1) = PDC3;
end


figure
hold on
plot(PDC1_Arr);
plot(PDC2_Arr);
plot(PDC3_Arr);
