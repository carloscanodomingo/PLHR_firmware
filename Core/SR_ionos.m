
classdef SR_ionos
    %SR_IONOS Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        ionos_table;
        ionos_table_hour;
        sr_day_NS ;
        sr_day_EW;
    end
    
    methods
        function obj = SR_ionos()
            %SR_IONOS Construct an instance of this class
            %   Detailed explanation goes here
            ionos_table = table();
            ionos_table_hour = table();
             
            SR_ionos_day_array = load("SR_ionos_day_array.mat");
            obj.sr_day_NS = SR_ionos_day_array.day_array_NS;
            obj.sr_day_EW = SR_ionos_day_array.day_array_EW;
            clearvars("SR_ionos_day_array");
            
            
            %Get data Height F, F2, E, and Es
            % By day
            table_height = ArrayRelevantParameters.get_data_height("day");
            ionos_table.hF = table_height.hF;
            ionos_table.hF2 = table_height.hF2;
            ionos_table.hE = table_height.hE;
            ionos_table.hEs = table_height.hEs;
            clearvars("table_height");
            
            % By hour
            %table_height_hour = ArrayRelevantParameters.get_data_height("hour");
            %ionos_table_hour.hF = table_height_hour.hF;
            %ionos_table_hour.hF2 = table_height_hour.hF2;
            %ionos_table_hour.hE = table_height_hour.hE;
            %ionos_table_hour.hEs = table_height_hour.hEs;
            clearvars("table_height_hour");
            
            %% GET TOTAL ELECTRON CONTENT 
            ionos_table.tec = ArrayRelevantParameters.get_data_tec("day");
            %ionos_table_hour.tec = ArrayRelevantParameters.get_data_tec("hour");
            
            %% GET SUNSPOT
            table_sunspot = ArrayRelevantParameters.get_data_sunspots();
            ionos_table.sunspot = table_sunspot.sunspot_total;
            ionos_table.sunspotN = table_sunspot.sunspot_north;
            ionos_table.sunspotS = table_sunspot.sunspot_south;
            
            %% GET GEOMAGNETIC INDEX
            table_geomagnetic_index = ArrayRelevantParameters.get_data_geomagnetic_index();
            ionos_table.Kp = table_geomagnetic_index.Kp;
            ionos_table.Ap = table_geomagnetic_index.Ap;
            
            %% GET LIGHTNING ACTIVITY
            ionos_table.Lightning = ArrayRelevantParameters.get_data_lightning();
            
            %% GET GLOBAL TEMPERATURE 
            ionos_table.Temp = ArrayRelevantParameters.get_data_global_temperature();
            
            
           %% GET SOLAR FLUX
           table_solar_flux = ArrayRelevantParameters.get_data_solar_flux();
           ionos_table.SolarFlux = table_solar_flux.sfluxFluxad;
           
            %% GET IRI2016 DATA
            ionos_table.NeD = ArrayRelevantParameters.get_data_iri2016();
            %%
            obj.ionos_table = ionos_table;
            obj.ionos_table_hour = ionos_table_hour;
            
        end
        
  
        function [output_table, output_matrix] = get_table_interval(obj,component, magnitude, threshold_p_value)
             arguments
                 obj,
                component {mustBeMember(component,["NS","EW"])}
                magnitude {mustBeMember(magnitude,["f","b"])}
                threshold_p_value 
             end
            
            if component == "NS"
                sr_day =   obj.sr_day_NS;
            elseif component == "EW"
                sr_day =   obj.sr_day_EW;
            end
            output_table = table();%
            output_matrix = cell(size(obj.ionos_table,2)- 2, SR_config.max_sr_mode);
            for index_var = 1:size(obj.ionos_table,2)
                
                    current_var = obj.ionos_table{1,index_var};
                    current_var_name = obj.ionos_table.Properties.VariableNames{index_var};
                    [matrix_corr, p_values] = current_var.find_correlation(sr_day, magnitude);
                    matrix_corr = cell2mat(matrix_corr);
                    p_values = cell2mat(p_values);
                    for index_mode =1:size(matrix_corr,2)
                        interval_array = [];
                        current_mode_corr = matrix_corr(:,index_mode);
                        current_mode_p_values = p_values(:,index_mode);
                        
                        %% Discard P values above threshold
                        discard = find(current_mode_p_values > threshold_p_value);
                        current_mode_corr(discard) = 0;
                        start_interval = 1;
                        sign_previous = sign(current_mode_corr(1));
                        length_interval = 0;
                        acum_corr = 0;
                        for index_hour = 1:length(current_mode_corr)
                            sign_current = sign(current_mode_corr(index_hour));
                            if current_mode_corr(index_hour) ~= 0 
                                if (length_interval == 0 ||  sign_current == sign_previous)
                                    length_interval = length_interval + 1;
                                    acum_corr = current_mode_corr(index_hour) + acum_corr;
                                end
                            else   
                                if length_interval ~= 0
                                    
                                    interval.start_interval = start_interval;
                                    interval.length = length_interval;
                                    interval.mean = acum_corr / length_interval;
                                    interval_array = [interval_array, interval];
                                 end
                            start_interval = index_hour + 1;
                            length_interval = 0;
                            acum_corr = 0;
                            end
                            sign_previous = sign_current;
                        end
                        if length_interval > 0
                               interval.start_interval = start_interval;
                                interval.length = length_interval;
                                interval.mean = acum_corr / length_interval;
                                interval_array = [interval_array, interval];
                        end
                        %% Check if the first interval is the continuation of the last interval
                        if length(interval_array) >= 1
                            if( length_interval ~= 0 && interval_array(1).start_interval == 1)
                                % Check if the two value has the same sign
                                if sign(interval_array(end).mean) == sign(interval_array(1).mean)
                                    interval_array(end).length = interval_array(1).length + length_interval;
                                    interval_array = interval_array(2:end);
                                end
                            end
                        end
                        interval_var_array{index_mode} = interval_array;
                        output_matrix{index_var, index_mode} = interval_array;
                    end
                    output_table = addvars(output_table, interval_var_array, 'NewVariableNames', current_var_name);      
            end
        end
        function plot(obj, component, magnitude, variable, type, selected_hours)
            arguments
            obj,
            component {mustBeMember(component,["NS","EW"])}
            magnitude {mustBeMember(magnitude,["f","b"])}           
            variable 
            type {mustBeMember(type,["correlation","hour"])}
            selected_hours
            end
            if variable ~= [obj.ionos_table.Properties.VariableNames "all"]
                display( variable + " is not a field of the table");
                for index_var = 1:size(obj.ionos_table.Properties.VariableNames,2)
                    current_name = obj.ionos_table.Properties.VariableNames{index_var};
                    display(current_name);
               
                
                end
                return 
            end
            if component == "NS"
                sr_day =   obj.sr_day_NS;
            elseif component == "EW"
                sr_day =   obj.sr_day_NS;
            end
            if variable == "all"
                
                for index_var = 1:length(obj.ionos_table.Properties.VariableNames)
                    if type == "correlation"
                        obj.ionos_table{:, index_var}.save_correlation(sr_day, magnitude);
                    elseif type == "hour"
                        obj.ionos_table{:, index_var}.save_time_distribution_hour( sr_day, selected_hours, magnitude);
                    end
                end
                   
            
            else
                if type == "correlation"
                    obj.ionos_table.(variable).save_correlation(sr_day, magnitude);
                elseif type == "hour"
                    obj.ionos_table.(variable).save_time_distribution_hour( sr_day, selected_hours, magnitude);
                end
            end
            
            
            
        end
        function [output_table,data] = export_data(obj)
            output_table = table();
            
            for index_param = 1:size(obj.ionos_table,2)
                current_var = obj.ionos_table{:,index_param};
                output_struct.(current_var.name) = current_var.get_mean_array()';
                output_table = addvars(output_table,current_var.get_mean_array()', 'NewVariableNames',current_var.name);
            end
            data = table2struct(output_table);
            data = output_struct;
            save("R/ionosphere_data.mat", "data");

            clearvars("data")
        
            freq_matrix = obj.sr_day_NS.relevant_parameter.frequency;
            intensity_matrix = obj.sr_day_NS.relevant_parameter.intensity;

            for index_row = 1:size(freq_matrix,1)
                for index_col = 1:size(freq_matrix,2)
                    %% WORKAROUND JUST FOR NOW
                    freq_array = [freq_matrix{index_row,index_col}.mean];
                    int_array = [intensity_matrix{index_row,index_col}.mean];
                    if (size(freq_array,2) == 59)
                        data.freq(index_row,index_col,:) = [freq_array,freq_array(:,end)];
                        data.inte(index_row,index_col,:) = [int_array,int_array(:,end)];
                    else
                        data.freq(index_row,index_col,:) = freq_array;
                        data.inte(index_row,index_col,:) = int_array;
                    end
                    
                end
            end

            save("R/srday_data.mat","data");
        end
        function print_table(obj, component, magnitude, type_of_print, threshold_p_value, format) 
            arguments
                 obj,
                component {mustBeMember(component,["NS","EW"])}
                magnitude {mustBeMember(magnitude,["f","b"])}
                type_of_print {mustBeMember(type_of_print,["full", "total_hour"])}
                threshold_p_value
                format {mustBeMember(format,["plain", "latex"])}
            end
            if format == "latex"
                values_separator = " & ";
                end_of_line = " \\\\ \n";
                end_of_var = "\\hline";
                percent = "";%"\\%%";
                if type_of_print == "full"
                    separation = "%13s";
                    lines_per_var = 4;
               elseif type_of_print == "total_hour"
                    lines_per_var = 1;
                    separation = "%9s";
             end
            elseif format == "plain"
                values_separator = "\t";
                end_of_line = "\n";
                percent = "";% "%%";
                end_of_var = "-------------------------------------------------------------------------------------------------------------------------------";
                                if type_of_print == "full"
                    separation = "%12s";
                    lines_per_var = 4;
               elseif type_of_print == "total_hour"
                    lines_per_var = 1;
                    separation = "%8s";
             end
            end

             [table_interval, matrix_interval] = get_table_interval(obj,component, magnitude,threshold_p_value);
             path = SR_config.base_path_linux + "table_sr_ionos_" + component+ "_" + magnitude  + type_of_print + format + ".txt"
             fileID = fopen(path,'w');
             fprintf(fileID,'%20s' + values_separator," ");
             for index_string = 1:(size(matrix_interval,2))
                fprintf(fileID, separation ,"Mode " + num2str(index_string));
                if index_string ~= (size(matrix_interval,2))
                    fprintf(fileID, values_separator);
                end
             end
        
             fprintf(fileID, end_of_line+end_of_var + "\n");

            for index_row = 1:size(matrix_interval,1)
                
                current_var_name = table_interval.Properties.VariableNames{index_row};
                max_struct_array = [];
                for index_check_lines = 1:(size(matrix_interval,2))
                    current_cell = matrix_interval{index_row, index_check_lines};
                    max_struct_array = [max_struct_array, size(current_cell,2)];
                end
                if type_of_print == "full"
                    max_of_gap  = max(max_struct_array);
                else
                    max_of_gap = 1;
                end
                
                for index_gap = 1:max_of_gap
                    if index_gap == 1
                        fprintf(fileID,'%20s' + values_separator,current_var_name);
                    else
                        fprintf(fileID,'%20s' + values_separator,current_var_name);
                        %fprintf(fileID,'%20s' + values_separator);
                    end
                    
                    for index_column = 1:size(matrix_interval,2)
                        current_cell = matrix_interval{index_row, index_column};
                        if type_of_print == "full"
                            if index_gap <= size(current_cell,2)
                                current_struct = current_cell(index_gap);
                                start_hour = current_struct.start_interval;
                                end_hour = mod(current_struct.start_interval + current_struct.length, 24);
                                value = round(current_struct.mean * 100) ;
                                fprintf(fileID,'[%2d-%2d] %3d' + percent ,start_hour, end_hour, value);
 
                            else 
                                fprintf(fileID, separation," ");
                            end
                               if index_column ~= size(matrix_interval,2)
                                fprintf(fileID,values_separator);
                            end
                        elseif type_of_print == "total_hour"
                            
                            acum_hours = 0;
                            acum_value = 0;
                            if size(current_cell,2) ~= 0
                                for index_struct = 1:size(current_cell,2)
                                    current_struct = current_cell(index_struct);
                                    acum_hours = acum_hours + current_struct.length;
                                    acum_value = acum_value + abs(current_struct.mean * current_struct.length);
                                   
                                end
                                 fprintf(fileID,'%2d - %2d' + percent ,acum_hours, round(acum_value/acum_hours  * 100));
                            else
                                fprintf(fileID,separation ," ");
                            end
                           if index_column ~= size(matrix_interval,2)
                                fprintf(fileID,values_separator);
                            end
                                
                            
                        end
                    end
                    fprintf(fileID,end_of_line);
                end
                 fprintf(fileID, end_of_var + "\n");
            end
            fclose(fileID);
        end
    end
end

%{
srclassdef SR_ionos
    %SR_IONOS Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        ionos_table;
        ionos_table_hour;
        sr_day_NS ;
        sr_day_EW;
    end
    
    methods
        function obj = SR_ionos()
            %SR_IONOS Construct an instance of this class
            %   Detailed explanation goes here
            ionos_table = table();
            ionos_table_hour = table();
             
            SR_ionos_day_array = load("SR_ionos_day_array.mat");
            obj.sr_day_NS = SR_ionos_day_array.day_array_NS;
            obj.sr_day_EW = SR_ionos_day_array.day_array_EW;
            clearvars("SR_ionos_day_array");
            
            
            %Get data Height F, F2, E, and Es
            % By day
            table_height = ArrayRelevantParameters.get_data_height("day");
            ionos_table.hF = table_height.hF;
            ionos_table.hF2 = table_height.hF2;
            ionos_table.hE = table_height.hE;
            ionos_table.hEs = table_height.hEs;
            clearvars("table_height");
            
            % By hour
            %table_height_hour = ArrayRelevantParameters.get_data_height("hour");
            %ionos_table_hour.hF = table_height_hour.hF;
            %ionos_table_hour.hF2 = table_height_hour.hF2;
            %ionos_table_hour.hE = table_height_hour.hE;
            %ionos_table_hour.hEs = table_height_hour.hEs;
            clearvars("table_height_hour");
            
            %% GET TOTAL ELECTRON CONTENT 
            ionos_table.tec = ArrayRelevantParameters.get_data_tec("day");
            %ionos_table_hour.tec = ArrayRelevantParameters.get_data_tec("hour");
            
            %% GET SUNSPOT
            table_sunspot = ArrayRelevantParameters.get_data_sunspots();
            ionos_table.sunspot = table_sunspot.sunspot_total;
            ionos_table.sunspotN = table_sunspot.sunspot_north;
            ionos_table.sunspotS = table_sunspot.sunspot_south;
            
            %% GET GEOMAGNETIC INDEX
            table_geomagnetic_index = ArrayRelevantParameters.get_data_geomagnetic_index();
            ionos_table.Kp = table_geomagnetic_index.Kp;
            ionos_table.Ap = table_geomagnetic_index.Ap;
            
            %% GET LIGHTNING ACTIVITY
            ionos_table.Lightning = ArrayRelevantParameters.get_data_lightning();
            
            %% GET GLOBAL TEMPERATURE 
            ionos_table.Temp = ArrayRelevantParameters.get_data_global_temperature();
            
            
           %% GET SOLAR FLUX
           table_solar_flux = ArrayRelevantParameters.get_data_solar_flux();
           ionos_table.SolarFlux = table_solar_flux.sfluxFluxad;
           
            %% GET IRI2016 DATA
            ionos_table.NeD = ArrayRelevantParameters.get_data_iri2016();
            %%
            obj.ionos_table = ionos_table;
            obj.ionos_table_hour = ionos_table_hour;
            
        end
        
  
        function [output_table, output_matrix] = get_table_interval(obj,component, magnitude, threshold_p_value)
             arguments
                 obj,
                component {mustBeMember(component,["NS","EW"])}
                magnitude {mustBeMember(magnitude,["f","b"])}
                threshold_p_value 
             end
            
            if component == "NS"
                sr_day =   obj.sr_day_NS;
            elseif component == "EW"
                sr_day =   obj.sr_day_EW;
            end
            output_table = table();%
            output_matrix = cell(size(obj.ionos_table,2)- 2, SR_config.max_sr_mode);
            for index_var = 1:size(obj.ionos_table,2)
                
                    current_var = obj.ionos_table{1,index_var};
                    current_var_name = obj.ionos_table.Properties.VariableNames{index_var};
                    [matrix_corr, p_values] = current_var.find_correlation(sr_day, magnitude);
                    matrix_corr = cell2mat(matrix_corr);
                    p_values = cell2mat(p_values);
                    for index_mode =1:size(matrix_corr,2)
                        interval_array = [];
                        current_mode_corr = matrix_corr(:,index_mode);
                        current_mode_p_values = p_values(:,index_mode);
                        
                        %% Discard P values above threshold
                        discard = find(current_mode_p_values > threshold_p_value);
                        current_mode_corr(discard) = 0;
                        start_interval = 1;
                        sign_previous = sign(current_mode_corr(1));
                        length_interval = 0;
                        acum_corr = 0;
                        for index_hour = 1:length(current_mode_corr)
                            sign_current = sign(current_mode_corr(index_hour));
                            if current_mode_corr(index_hour) ~= 0 
                                if (length_interval == 0 ||  sign_current == sign_previous)
                                    length_interval = length_interval + 1;
                                    acum_corr = current_mode_corr(index_hour) + acum_corr;
                                end
                            else   
                                if length_interval ~= 0
                                    
                                    interval.start_interval = start_interval;
                                    interval.length = length_interval;
                                    interval.mean = acum_corr / length_interval;
                                    interval_array = [interval_array, interval];
                                 end
                            start_interval = index_hour + 1;
                            length_interval = 0;
                            acum_corr = 0;
                            end
                            sign_previous = sign_current;
                        end
                        if length_interval > 0
                               interval.start_interval = start_interval;
                                interval.length = length_interval;
                                interval.mean = acum_corr / length_interval;
                                interval_array = [interval_array, interval];
                        end
                        %% Check if the first interval is the continuation of the last interval
                        if length(interval_array) >= 1
                            if( length_interval ~= 0 && interval_array(1).start_interval == 1)
                                % Check if the two value has the same sign
                                if sign(interval_array(end).mean) == sign(interval_array(1).mean)
                                    interval_array(end).length = interval_array(1).length + length_interval;
                                    interval_array = interval_array(2:end);
                                end
                            end
                        end
                        interval_var_array{index_mode} = interval_array;
                        output_matrix{index_var, index_mode} = interval_array;
                    end
                    output_table = addvars(output_table, interval_var_array, 'NewVariableNames', current_var_name);      
            end
        end
        function plot(obj, component, magnitude, variable, type, selected_hours)
            arguments
            obj,
            component {mustBeMember(component,["NS","EW"])}
            magnitude {mustBeMember(magnitude,["f","b"])}           
            variable 
            type {mustBeMember(type,["correlation","hour"])}
            selected_hours
            end
            if variable ~= [obj.ionos_table.Properties.VariableNames "all"]
                display( variable + " is not a field of the table");
                for index_var = 1:size(obj.ionos_table.Properties.VariableNames,2)
                    current_name = obj.ionos_table.Properties.VariableNames{index_var};
                    display(current_name);
               
                
                end
                return 
            end
            if component == "NS"
                sr_day =   obj.sr_day_NS;
            elseif component == "EW"
                sr_day =   obj.sr_day_NS;
            end
            if variable == "all"
                
                for index_var = 1:length(obj.ionos_table.Properties.VariableNames)
                    if type == "correlation"
                        obj.ionos_table{:, index_var}.save_correlation(sr_day, magnitude);
                    elseif type == "hour"
                        obj.ionos_table{:, index_var}.save_time_distribution_hour( sr_day, selected_hours, magnitude);
                    end
                end
                   
            
            else
                if type == "correlation"
                    obj.ionos_table.(variable).save_correlation(sr_day, magnitude);
                elseif type == "hour"
                    obj.ionos_table.(variable).save_time_distribution_hour( sr_day, selected_hours, magnitude);
                end
            end
            
            
            
        end
        function [output_table,data] = export_data(obj)
            output_table = table();
            
            for index_param = 1:size(obj.ionos_table,2)
                current_var = obj.ionos_table{:,index_param};
                output_struct.(current_var.name) = current_var.get_mean_array()';
                output_table = addvars(output_table,current_var.get_mean_array()', 'NewVariableNames',current_var.name);
            end
            data = table2struct(output_table);
            data = output_struct;
            save("R/ionosphere_data.mat", "data");

            clearvars("data")
        
            freq_matrix = obj.sr_day_NS.relevant_parameter.frequency;
            intensity_matrix = obj.sr_day_NS.relevant_parameter.intensity;

            for index_row = 1:size(freq_matrix,1)
                for index_col = 1:size(freq_matrix,2)
                    %% WORKAROUND JUST FOR NOW
                    freq_array = [freq_matrix{index_row,index_col}.mean];
                    int_array = [intensity_matrix{index_row,index_col}.mean];
                    if (size(freq_array,2) == 59)
                        data.freq(index_row,index_col,:) = [freq_array,freq_array(:,end)];
                        data.inte(index_row,index_col,:) = [int_array,int_array(:,end)];
                    else
                        data.freq(index_row,index_col,:) = freq_array;
                        data.inte(index_row,index_col,:) = int_array;
                    end
                    
                end
            end

            save("R/srday_data.mat","data");
        end
        function print_table(obj, component, magnitude, type_of_print, threshold_p_value, format) 
            arguments
                 obj,
                component {mustBeMember(component,["NS","EW"])}
                magnitude {mustBeMember(magnitude,["f","b"])}
                type_of_print {mustBeMember(type_of_print,["full", "total_hour"])}
                threshold_p_value
                format {mustBeMember(format,["plain", "latex"])}
            end
            if format == "latex"
                values_separator = " & ";
                end_of_line = " \\\\ \n";
                end_of_var = "\\hline";
                percent = "\\%%";
                if type_of_print == "full"
                    separation = "%13s";
                    lines_per_var = 4;
               elseif type_of_print == "total_hour"
                    lines_per_var = 1;
                    separation = "%9s";
             end
            elseif format == "plain"
                values_separator = "\t";
                end_of_line = "\n";
                percent = "%%";
                end_of_var = "-------------------------------------------------------------------------------------------------------------------------------";
                                if type_of_print == "full"
                    separation = "%12s";
                    lines_per_var = 4;
               elseif type_of_print == "total_hour"
                    lines_per_var = 1;
                    separation = "%8s";
             end
            end

             [table_interval, matrix_interval] = get_table_interval(obj,component, magnitude,threshold_p_value);
             path = SR_config.base_path_linux + "table_sr_ionos_" + component+ "_" + magnitude + " " + type_of_print + "_" + threshold_p_value + "_" + format + ".txt";
             fileID = fopen(path,'w');
             fprintf(fileID,'%20s' + values_separator," ");
             for index_string = 1:(size(matrix_interval,2))
                fprintf(fileID, separation ,"SR MODE " + num2str(index_string));
                if index_string ~= (size(matrix_interval,2))
                    fprintf(fileID, values_separator);
                end
             end
        
             fprintf(fileID, end_of_line+end_of_var + "\n");

            for index_row = 1:size(matrix_interval,1)
                
                current_var_name = table_interval.Properties.VariableNames{index_row};
                max_struct_array = [];
                for index_check_lines = 1:(size(matrix_interval,2))
                    current_cell = matrix_interval{index_row, index_check_lines};
                    max_struct_array = [max_struct_array, size(current_cell,2)];
                end
                if type_of_print == "full"
                    max_of_gap  = max(max_struct_array);
                else
                    max_of_gap = 1;
                end
                
                for index_gap = 1:max_of_gap
                    fprintf(fileID,'%20s' + values_separator,current_var_name);
                    for index_column = 1:size(matrix_interval,2)
                        current_cell = matrix_interval{index_row, index_column};
                        if type_of_print == "full"
                            if index_gap <= size(current_cell,2)
                                current_struct = current_cell(index_gap);
                                start_hour = current_struct.start_interval;
                                end_hour = mod(current_struct.start_interval + current_struct.length, 24);
                                value = round(current_struct.mean * 100) ;
                                fprintf(fileID,'[%2d-%2d] %3d' + percent ,start_hour, end_hour, value);
 
                            else 
                                fprintf(fileID, separation," ");
                            end
                               if index_column ~= size(matrix_interval,2)
                                fprintf(fileID,values_separator);
                            end
                        elseif type_of_print == "total_hour"
                            
                            acum_hours = 0;
                            acum_value = 0;
                            if size(current_cell,2) ~= 0
                                for index_struct = 1:size(current_cell,2)
                                    current_struct = current_cell(index_struct);
                                    acum_hours = acum_hours + current_struct.length;
                                    acum_value = acum_value + abs(current_struct.mean * current_struct.length);
                                   
                                end
                                 fprintf(fileID,'%2d - %2d' + percent ,acum_hours, round(acum_value/acum_hours  * 100));
                            else
                                fprintf(fileID,separation ," ");
                            end
                           if index_column ~= size(matrix_interval,2)
                                fprintf(fileID,values_separator);
                            end
                                
                            
                        end
                    end
                    fprintf(fileID,end_of_line);
                end
                 fprintf(fileID, end_of_var + "\n");
            end
            fclose(fileID);
        end
    end
end

>>>>>>> c5e9f21d1e3d9772f190127e2aa6c2df467d04ca
%}
