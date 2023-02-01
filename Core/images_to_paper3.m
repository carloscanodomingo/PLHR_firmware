
sr_ionos =SR_ionos();

sr_data = load('SR_2020_03_NS_v_5.mat');
sr_data = SR_peak_process_array.add_station(SR_2018_05_NS, "ALM");
SR_peak_process.plot(sr_data(760), "pT", "lorentz", "Normal", 1)
save_fig("wide", "example_lorentz_paper_3", "png")
% Figure 1
% a Figure 8
sr_ionos.plot("NS", "f", "Lightning","hour",11)
%b, Figure
sr_ionos.plot("NS", "f", "hE","hour",11)
% c
sr_ionos.plot("NS", "f", "tec","hour",11)



% Table I
sr_ionos.print_table("NS","f","total_hour",0.01, "latex")

%Figure 2,3,4,5,6,7
sr_ionos.plot("NS", "f", "all","correlation",11)

sr_ionos.print_table("NS","f","full",0.001, "latex")

% Figure 8
sr_ionos.plot("NS", "f", "Lightning","hour",11)
% Figure 9
sr_ionos.plot("NS", "f", "hE","hour",10)