figure
subplot(2,1,1)
hold on
plot(nome(:,4),nome(:,1)) % real
plot(nome(:,4),nome(:,2)) % ref
xlabel('time_{[ms]}');
ylabel('LUX')
yyaxis right
ylabel('pwm');
plot(nome(:,4),nome(:,3)) % pwm

%%
subplot(2,1,2)
hold on
plot(ffd_off(:,4),ffd_off(:,1)) % real
plot(ffd_off(:,4),ffd_off(:,2)) % ref
xlabel('time_{[ms]}');
ylabel('LUX')
yyaxis right
ylabel('pwm')
plot(ffd_off(:,4),ffd_off(:,3)) % pwm

figure
fed = 75;

ref = 600;
for i = 1:4

subplot(2,4,i)
hold on
space = ref-20:ref+3*fed;
plot(nome(space,4),nome(ref-20:ref+fed*3,1));
plot(nome(space,4),nome(ref-20:ref+fed*3,2));
xlabel('time_{[ms]}');
ylabel('LUX')



subplot(2,4,4+i)
hold on
plot(ffd_off(space,4),ffd_off(ref-20:ref+fed*3,1));
plot(ffd_off(space,4),ffd_off(ref-20:ref+fed*3,2));
xlabel('time_{[ms]}');
ylabel('LUX')


ref = ref + 300;

end


figure
subplot(2,1,1)
hold on
plot(wind_up(:,4),wind_up(:,1)) % real
plot(wind_up(:,4),wind_up(:,2)) % ref
xlabel('time_{[ms]}');
ylabel('LUX')
yyaxis right
ylabel('pwm');
plot(wind_up(:,4),wind_up(:,3)-85) % pwm
%%
figure
subplot(2,1,1)
hold on
plot(wind_up(:,4),wind_up(:,1)) % real
plot(wind_up(:,4),wind_up(:,2)) % ref
xlabel('time_{[ms]}');
ylabel('LUX')

subplot(2,1,2)
hold on
plot(wind_off(:,4),wind_off(:,1)) % real
plot(wind_off(:,4),wind_off(:,2)) % ref
xlabel('time_{[ms]}');
ylabel('LUX')



