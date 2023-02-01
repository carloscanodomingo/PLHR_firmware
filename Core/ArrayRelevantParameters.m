classdef ArrayRelevantParameters
    %PROCCESSARRAYRELEVANTPARAMETERS Summary of this class goes here
    %   Detailed explanation goes here
    properties
        array_relevant_parameter;
        timetable_day;
        data_type;
        label;
        name;
        type;
    end
    
    properties(Constant) 
        path = "paper_3";
        format = "pdf";
        path_correlation = "correlation";
        path_time= "time";
        path_hour = "hour"
        path_day = "day"
        
    end
    methods
        function string_output = get_save_path_evolution(obj, magnitude, component, start_hour, sr_mode)
           arguments
                obj
                magnitude {mustBeMember(magnitude,["f","b"])}
                component string
                start_hour double
                sr_mode double
            end
                     string_output = fullfile(obj.path, obj.path_time, component, magnitude,obj.name, num2str(start_hour), obj.name +  "_"  +  start_hour +  "_" +  component + "_SR_" + sr_mode +  "_" + magnitude);
        end
        
        function string_output = get_save_path_correlation(obj, magnitude, component, sr_mode)
           arguments
                obj
                magnitude {mustBeMember(magnitude,["f","b"])}
                component string
                sr_mode double
           end
            if obj.type == "hour"
                     string_output = fullfile(obj.path, obj.path_correlation, component, magnitude,obj.name, num2str(sr_mode), obj.name +  "_"  +  sr_mode +  "_" +  component + "_SR_" + sr_mode + "_" + magnitude);
            elseif obj.type == "day"
                     string_output = fullfile(obj.path, obj.path_correlation, component, magnitude,obj.name, obj.name  +  "_" +  component  + "_" + magnitude);
            end
            end
        function Object = ArrayRelevantParameters(time_vector, data_vector, data_type, label, name, type)
           arguments
                time_vector (1,:) datetime
                data_vector (1,:) double
                data_type string
                label string
                name string
                type {mustBeMember(type,["day","hour"])}
           end
            if ~isequal(size(time_vector),size(data_vector))
                eid = 'Size:notEqual';
                msg = 'Size of first input must equal size of second input.';
                throwAsCaller(MException(eid,msg))
            end
            
            
            Object.data_type = data_type;
            Object.label = label;
            Object.name = name;
            Object.type = type;
            if Object.type == "day"
                [Object.array_relevant_parameter, Object.timetable_day] = ArrayRelevantParameters.create_array_day(time_vector, data_vector); 
            elseif Object.type == "hour"
                Object.array_relevant_parameter = ArrayRelevantParameters.create_array_hour(time_vector, data_vector); 
            end
        end
        
        function output_matrix = save_correlation(obj, sr_day, magnitude)
            arguments
                obj
                sr_day (1,1) SR_day_array
                magnitude {mustBeMember(magnitude,["f","b"])}
            end
            if obj.type == "day"
                output_matrix = save_correlation_day(obj, sr_day, magnitude);
            elseif obj.type == "hour"
                output_matrix = save_correlation_hour(obj, sr_day, magnitude);
            end
        end

        function output_matrix = save_time_distribution_hour(obj, sr_day, selected_hours, magnitude) 
             arguments
                obj
                sr_day (1,1) SR_day_array
                selected_hours (1,:) double
                magnitude {mustBeMember(magnitude,["f","b"])}
            end
            if obj.type ==  "hour"
                current_relevant_parameter = obj.array_relevant_parameter{selected_hours(2)};
                hour_add = num2str(selected_hours(2)) + ":00";
            else 
                current_relevant_parameter = obj.array_relevant_parameter;
                hour_add = "";
            end
            
                sr_day_relevant = ArrayRelevantParameters.select_magnitude_relevant(sr_day, magnitude);
                output_matrix = sr_day_relevant;
                for index_mode=1:size(sr_day_relevant,1)
                    close
                    fig= figure;
                    color_left =  [0.8 0.4 0.2];
                    color_right = [0.3 0.8 0.6];
                    set(fig,'defaultAxesColorOrder',[color_left; color_right]);
                    current_mode_hour = sr_day_relevant{index_mode, selected_hours(1)};
                    yyaxis left
                    
                    plot([current_mode_hour.date], [current_mode_hour.mean], "LineWidth", 2);
                    hold on;
                    plot([current_mode_hour.date], [current_mode_hour.mean], "LineWidth", 1, "Color", "k");
                    ylabel("Frequency Variation");
                    yyaxis right
                    
                    plot([current_relevant_parameter.date], [current_relevant_parameter.mean], "LineWidth", 2);
                    hold on;
                    plot([current_relevant_parameter.date], [current_relevant_parameter.mean], "LineWidth", 1, "Color", "k");
                    if  SR_config_base.SR_paper_3_show_title == true
                        title(sr_day.component + " " + magnitude + " Time evolution - " +  selected_hours(1)+ ":00 -  SR Mode " + index_mode + " VS " + obj.data_type + hour_add);
                    else
                        title("")
                    end
                    ylabel(obj.label + " variation " + magnitude );
                    path_name = obj.get_save_path_evolution(magnitude, sr_day.component , selected_hours(1), index_mode)
                    
                     save_fig("square", path_name, obj.format)
                end             
        end
        function test_granger_result = test_granger(obj, sr_day, magnitude)
            arguments
                obj
                sr_day (1,1) SR_day_array
                magnitude {mustBeMember(magnitude,["f","b"])}
            end
            sr_day_array_relevant  = ArrayRelevantParameters.select_magnitude_relevant(sr_day, magnitude);
            for index_mode = size(sr_day_array_relevant,1)
                for index_hour_1= 1:size(sr_day_array_relevant,2)
                current_values = sr_day_array_relevant{index_mode, index_hour_1};
                        current_other_param = obj.array_relevant_parameter{1};
                            selected = ArrayRelevantParameters.select_same_time(current_values, current_other_param);
                            sr_variable = selected(1,:);
                            comparing_variable = selected(2,:);
                            [correlation,p]  =  corr(normalize(diff(selected')));
                p_values{index_mode} = p;
                correlation_values{index_mode} = correlation;
                end
            end
            
        end
    
    function [correlation_values, p_values, p_values_test] = find_correlation(obj, sr_day, magnitude)
            arguments
                obj
                sr_day (1,1) SR_day_array
                magnitude {mustBeMember(magnitude,["f","b"])}
            end
            sr_day_array_relevant  = ArrayRelevantParameters.select_magnitude_relevant(sr_day, magnitude);
            for index_mode = 1:size(sr_day_array_relevant,1)
                for index_hour_1= 1:size(sr_day_array_relevant,2)
                current_values = sr_day_array_relevant{index_mode, index_hour_1};
                    if length(obj.array_relevant_parameter) == 60
                        current_other = {obj.array_relevant_parameter};
                    else
                        current_other = obj.array_relevant_parameter;
                    end
                    for index_hour_2 = 1:length(current_other)
                        current_other_param = current_other{index_hour_2};
                        if ~isempty(current_other_param) & ~isempty(current_values) 
                            selected = ArrayRelevantParameters.select_same_time(current_values, current_other_param);
                            [correlation,p]  =  corr(normalize((selected')));
                            
                            % Test to see casualty
                            [h,pValue_test,stat,cValue,reg1,reg2] = egcitest((selected'));
                            p_values_mode(index_hour_1, index_hour_2) = (p(1,2));
                            correlation_values_mode(index_hour_1, index_hour_2) = correlation(1,2);
                            test_p_value(index_hour_1, index_hour_2)  = -log10(pValue_test);
                        else
                            p_values_mode(index_hour_1, index_hour_2) = 0;
                            correlation_values_mode(index_hour_1, index_hour_2) = 0;
                            test_p_value(index_hour_1, index_hour_2)  = 0;
                        end
                    end
                end
                p_values{index_mode} = p_values_mode;
                correlation_values{index_mode} = correlation_values_mode;
                p_values_test{index_mode}  = test_p_value;
            end
    end
    function [mean_array_month, time_table_day] = get_mean_array(obj)
        mean_array_month = [obj.array_relevant_parameter{1}.mean];
        mean_array_month = [obj.array_relevant_parameter{1}.mean];
        time_table_day = timetable([obj.timetable_day.current_datetime],...
                    [sr_ionos.ionos_table.hF.timetable_day.current_value.mean]')
   
    end
    end
    methods(Access = private)
        function output_matrix = save_correlation_hour(obj, sr_day, magnitude)
            arguments
                obj
                sr_day (1,1) SR_day_array
                magnitude {mustBeMember(magnitude,["f","b"])}
            end
           
            correlation_values = obj.find_correlation(sr_day, magnitude);
            output_matrix = correlation_values;
            for index_mode = 1:size(correlation_values,2)
                current_mode = correlation_values{index_mode};
                h = heatmap(current_mode, 'CellLabelColor', 'None'); colormap jet;
                h.NodeChildren(3).YDir='normal'; 
                caxis([-1,1]);
                if  SR_config_base.SR_paper_3_show_title == true
                    title("Matrix Correlation -  SR Modes " + magnitude + " "  + index_mode +  " VS " + obj.data_type + " " +  sr_day.component);
                else
                    title("")
                end
                xlabel(obj.label + " Hours");
                ylabel("SR Hours");
                full_path = get_save_path_correlation(obj, magnitude, sr_day.component ,  index_mode);
                save_fig("square", full_path, obj.format)
            end
        end
        
        function output_matrix = save_correlation_day(obj, sr_day, magnitude)
            arguments
                obj
                sr_day (1,1) SR_day_array
                magnitude {mustBeMember(magnitude,["f","b"])}
            end
            [correlation_values, p_values,  ~] = obj.find_correlation(sr_day, magnitude);
            output_matrix = correlation_values;
            close 
            f = figure;
            matrix_corr = cell2mat(correlation_values);
            h = heatmap(matrix_corr); colormap jet;
            h.NodeChildren(3).YDir='normal';
            h.CellLabelFormat = '%.2f';
            caxis([-1,1]);
            if  SR_config_base.SR_paper_3_show_title == true
                title("Matrix Correlation -  SR Modes "   + magnitude + " " + " VS " + obj.data_type + " " +  sr_day.component);
            else
                title("")
            end
            xlabel("SR modes")
            ylabel("SR Hours");
            warning('off')
            axs = struct(gca); %ignore warning that this should be avoided
            cb = axs.Colorbar;
            cb.Label.String = 'Pearson Coefficient';
            cb.FontSize = 10
            warning('on')
         full_path = get_save_path_correlation(obj, magnitude, sr_day.component ,  0);
         save_fig("square", full_path, obj.format)
             
                
             close 
             print_pvalues= 0
           if print_pvalues == 1
                f = figure();
                [X,Y]=meshgrid(1:size(matrix_corr,2),1:size(matrix_corr,1));
                matrix_pvalue = log10(cell2mat(p_values));
                h = imagesc(X(:), Y(:), matrix_corr);
                caxis([-1,1]);
                f.Children(1).YDir='normal';
                colormap jet;
                txt = sprintfc('%.1f', matrix_pvalue);
                h2 = text(X(:), Y(:), txt, 'horizontalalignment','center','verticalalignment','middle', 'BackgroundColor', 'w', 'Color', 'k', 'Margin', 0.0001, 'fontsize', 6);
                
                if  SR_config_base.SR_paper_3_show_title == true
                    title("P values Correlation -  SR Modes "  + magnitude + " " + " VS " + obj.data_type + " " +  sr_day.component);
                else
                    title("")
                end
                xlabel("SR modes")
                ylabel("SR Hours");
                
                
                 save_fig("square", full_path + "p_value", obj.format)
           end
            
        end

        
       
    end
    methods(Static, Access = private)
                function selected_magnitude = select_magnitude_relevant(sr_day, magnitude)
             arguments
                sr_day (1,1) SR_day_array
                magnitude {mustBeMember(magnitude,["f","b"])}
             end
             if magnitude == "f"
                 selected_magnitude = sr_day.relevant_parameter.frequency;
             elseif magnitude == "b"
                 selected_magnitude = sr_day.relevant_parameter.intensity;
             end
        end
    function [array_relevant_paramenters, timetable_day]= create_array_day(time_vector,data_vector)
            arguments
                time_vector (1,:) datetime
                data_vector (1,:) double
            end
            if ~isequal(size(time_vector),size(data_vector))
                eid = 'Size:notEqual';
                msg = 'Size of first input must equal size of second input.';
                throwAsCaller(MException(eid,msg))
            end
            % Remove empty vector
            select_remove_nan = (~isnan(data_vector) );
            data_vector = data_vector(select_remove_nan);
            time_vector = time_vector(select_remove_nan);

            % Initialize Monthly Vector
            array_relevant_paramenters = [];
            % Moving to TimeTable
            timetable_day = timetable();
            min_year = min(year(time_vector));
            max_year = max(year(time_vector));
            for index_year = min_year:max_year
                select_index_year = year(time_vector) == index_year;
                time_vector_year = time_vector(select_index_year);
                data_vector_year = data_vector(select_index_year);
                min_month = min(month(time_vector_year));
                max_month = max(month(time_vector_year));
                for index_month = min_month:max_month
                    select_index_month = month(time_vector_year) == index_month;
                    time_vector_month = time_vector_year(select_index_month);
                    data_vector_month = data_vector_year(select_index_month);
                    if ~isempty(data_vector_month)
                        array_relevant_paramenters = [array_relevant_paramenters, RelevantParameters(time_vector_month, data_vector_month)];
                    end
                    min_day = min(day(time_vector_month));
                    max_day = max(day(data_vector_month));
                   for index_day = min_day:max_day
                        select_index_day = day(time_vector_month) == index_day;
                        time_vector_day= time_vector_month(select_index_day);
                        data_vector_day = data_vector_month(select_index_day);
                        if ~isempty(data_vector_day)
                            current_datetime = datetime(index_year, index_month, index_day);
                            current_value = RelevantParameters(time_vector_day, data_vector_day);
                            timetable_day = [timetable_day; timetable(current_datetime, current_value) ];
                        end
                   end
                end
            end
        end
        
        function array_relevant_paramenters = create_array_hour(time_vector,data_vector)
            arguments
                time_vector (1,:) datetime
                data_vector (1,:) double
            end
            if ~isequal(size(time_vector),size(data_vector))
                eid = 'Size:notEqual';
                msg = 'Size of first input must equal size of second input.';
                throwAsCaller(MException(eid,msg))
            end
            for index_hour = 0:23
                select_hour = hour(time_vector) == index_hour;
                time_vector_hour = time_vector(select_hour);
                data_vector_hour = data_vector(select_hour);
                array_relevant_paramenters{index_hour + 1} = ArrayRelevantParameters.create_array_day(time_vector_hour, data_vector_hour);
            end
        end
        
end
    methods(Static)
        
        function output = select_same_time(array_relevant_param_1, array_relevant_param_2)
                 arguments
                    array_relevant_param_1 (1,:) RelevantParameters
                    array_relevant_param_2 (1,:) RelevantParameters
                 end
                time_values_1 = [array_relevant_param_1.date];
                time_values_2 = [array_relevant_param_2.date];
                common_times = intersect(time_values_1, time_values_2);
                
                for index_time = 1:length(common_times)
                    [~, index_1] = find(time_values_1 == common_times(index_time));
                    [~, index_2] = find(time_values_2 == common_times(index_time));
                    output(1, index_time) = [array_relevant_param_1(index_1).mean];
                    output(2, index_time) = [array_relevant_param_2(index_2).mean];
                end
        end
        

        
        
        function save_by_hour(filename, table_data) 

            for index_hour= 1:24
                figure(1)
                close 
                hold on
                for index_type = 1:size(table_data,2)
                current_option = table_data{1,index_type};
                current_option_hour = current_option{index_hour};
                if ~isempty(current_option_hour)
                    vector_time = [current_option_hour.date];
                    vector_data = [current_option_hour.mean];
                    hold on
                    plot(vector_time, vector_data,  'DisplayName',table_data.Properties.VariableNames{index_type}, 'LineWidth',2);
    
                end
                if  SR_config.SR_paper_3_show_title == true
                    title("FROM: " + num2str(index_hour - 1) + ":00    TO: " + index_hour + ":00");
                else
                    title("")
                end
                
                legend();
                ylim([0 200]);
                hold off
                end
                save_fig("square","img/paper_3/hour/h_" + index_hour +  "_" +filename , "png");
            end
        end
        function struct_var = get_struct() 
            struct_var.height_hour = ArrayRelevantParameters.get_data_height("hour");
            struct_var.height_day = ArrayRelevantParameters.get_data_height("day");
            struct_var.kp_ap = ArrayRelevantParameters.get_ap_kp();
            struct_var.tec_hour = ArrayRelevantParameters.get_data_tec("hour");
            struct_var.tec_day = ArrayRelevantParameters.get_data_tec("day");
            struct_var.global_temp = ArrayRelevantParameters.get_global_temperature();
            struct_var.lighting = ArrayRelevantParameters.get_lighting_day;
            struct_var.sunspot = ArrayRelevantParameters.get_sunspots();
            struct_var.solar_flux = ArrayRelevantParameters.get_solar_flux();
        end
        function table_sunspot =  get_data_sunspots()
                        %% Import data from text file
            % Script for importing data from the following text file:
            %
            %    filename: ionosphere_data_sunspot.csv
            %
            % 
        %{
         International Sunspot Number SN
        # The international sunspot number SN (written with subscript N) is given as the daily total sunspot number version 2.0 introduced in 2015.
        # The sunspot data is available under the licence CC BY-NC 4.0 from WDC-SILSO, Royal Observatory of Belgium, Brussels. Described in:
        # Clette, F., Lefevre, L., 2016. The New Sunspot Number: assembling all corrections. Solar Physics, 291, https://doi.org/10.1007/s11207-016-1014-y 
        # Note: the most recent values are preliminary and replaced by definitive values as soon as they become available.
        #
        %}
        %% Set up the Import Options and import the data
        opts = delimitedTextImportOptions("NumVariables", 14);

        % Specify range and delimiter
        opts.DataLines = [1, Inf];
        opts.Delimiter = ";";

        % Specify column names and types
        opts.VariableNames = ["year", "month", "day", "Var4", "sunspot_total", "sunspot_north", "sunspot_south", "Var8", "Var9", "Var10", "Var11", "Var12", "Var13", "Var14"];
        opts.SelectedVariableNames = ["year", "month", "day", "sunspot_total", "sunspot_north", "sunspot_south"];
        opts.VariableTypes = ["double", "double", "double", "string", "double", "double", "double", "string", "string", "string", "string", "string", "string", "string"];

        % Specify file level properties
        opts.ExtraColumnsRule = "ignore";
        opts.EmptyLineRule = "read";

        % Specify variable properties
        opts = setvaropts(opts, ["Var4", "Var8", "Var9", "Var10", "Var11", "Var12", "Var13", "Var14"], "WhitespaceRule", "preserve");
        opts = setvaropts(opts, ["Var4", "Var8", "Var9", "Var10", "Var11", "Var12", "Var13", "Var14"], "EmptyFieldRule", "auto");

        % Import the data
        sunspot_hemis = readtable("ionosphere_data_sunspot.csv", opts);


        % Remove unusefull years
        years = sunspot_hemis.year;
        sunspot_hemis = sunspot_hemis((years >= SR_config_base.start_year & years <= SR_config_base.end_year),:);
        time_vector = arrayfun(@(year,month, day) datetime(year, month, day),sunspot_hemis.year, sunspot_hemis.month, sunspot_hemis.day);

        sunspot_total = ArrayRelevantParameters(time_vector, sunspot_hemis.sunspot_total, "Total Sunspot", "Sum Sunspot Total", "sunspot_total", "day");
        sunspot_north = ArrayRelevantParameters(time_vector,  sunspot_hemis.sunspot_north, "North Sunspot", "Sum Sunspot North", "sunspot_north", "day");
        sunspot_south = ArrayRelevantParameters(time_vector, sunspot_hemis.sunspot_south, "South Sunspot", "Sum Sunspot South", "sunspot_south", "day");


        table_sunspot = table(sunspot_total, sunspot_north, sunspot_south);

    end
   
    function  output_object =  get_data_geomagnetic_index()
                        %% Import data from text file
        %{
        # Kp, ap and Ap
        # The three-hourly equivalent planetary amplitude ap is derived from Kp and the daily equivalent planetary amplitude Ap is the daily mean of ap.
        # Kp is unitless. Ap and ap are unitless and can be multiplied by 2 nT to yield the average geomagnetic disturbance at 50 degree geomagnetic latitude.
        # Kp, ap and Ap were introduced by Bartels (1949, 1957) and are produced by Geomagnetic Observatory Niemegk, GFZ German Research Centre for Geosciences.
        # Described in: Matzka et al. (2021), see reference above.
        # Data publication: Matzka, J., Bronkalla, O., Tornow, K., Elger, K. and Stolle, C., 2021. Geomagnetic Kp index. V. 1.0. GFZ Data Services, 
        # https://doi.org/10.5880/Kp.0001
        # Note: the most recent values are nowcast values and will be replaced by definitive values as soon as they become available.
        # 
        %}

        %% Set up the Import Options and import the data
        opts = delimitedTextImportOptions("NumVariables", 19);

        % Specify range and delimiter
        opts.DataLines = [1, Inf];
        opts.Delimiter = " ";

        % Specify column names and types
        opts.VariableNames = ["date", "Kp1", "Kp2", "Kp3", "Kp4", "Kp5", "Kp6", "Kp7", "Kp8", "Kp", "Ap1", "Ap2", "Ap3", "Ap4", "Ap5", "Ap6", "Ap7", "Ap8", "Ap"];
        opts.VariableTypes = ["string", "string", "string", "string", "string", "string", "string", "string", "string", "string", "double", "double", "double", "double", "double", "double", "double", "double", "double"];

        % Specify file level properties
        opts.ExtraColumnsRule = "ignore";
        opts.EmptyLineRule = "read";
        opts.ConsecutiveDelimitersRule = "join";
        opts.LeadingDelimitersRule = "ignore";


        variable_names_ap = ["Ap1", "Ap2", "Ap3", "Ap4", "Ap5", "Ap6", "Ap7", "Ap8", "Ap"];
        % Specify variable properties
        variable_names_kp = ["Kp1", "Kp2", "Kp3", "Kp4", "Kp5", "Kp6", "Kp7", "Kp8", "Kp"];
        opts = setvaropts(opts, variable_names_kp, "EmptyFieldRule", "auto");



        filename = "ionosphere_data_kp_ap.txt";
        % Import data
        table_kp_ap = readtable(filename, opts);
        vector_time = arrayfun(@(a) datetime(a, 'InputFormat',"yyyyMMdd"),table_kp_ap.date);
        output_object = table();
        for index_kp = 1:length(variable_names_kp)
            current_name = variable_names_kp(index_kp);
            values_string = table_kp_ap.(current_name);
            values_number = zeros(1, length(values_string));
            for index_string = 1:length(values_string)
                current_string = values_string(index_string);
                if contains(current_string,"+")
                    only_number = split(current_string, "+");
                    values_number(index_string) = str2double(only_number(1)) + 1/3;
                elseif contains(current_string,"-")
                    only_number = split(current_string, "-");
                    values_number(index_string) = str2double(only_number(1)) - 1/3;
                else
                     values_number(index_string) = str2double(current_string);
                end
            end
            output_object.(current_name) =  ArrayRelevantParameters(vector_time, values_number, "Total " + current_name ,current_name + " Day", current_name + "_day", "day");
        end
        for index_ap = 1:length(variable_names_ap)
            current_name = variable_names_ap(index_ap);
            output_object.(current_name) =  ArrayRelevantParameters(vector_time, table_kp_ap.(current_name), "Total " + current_name ,current_name + " Day", current_name + "_day", "day");
        end

          end
          
          
          
          
          
          %%
          function height_relevant_parameters =  get_data_height(option)
              arguments
                option {mustBeMember(option,["day","hour"])}
            end
          %% Set up the Import Options and import the data
        opts = delimitedTextImportOptions("NumVariables", 17);

        % Specify range and delimiter
        opts.DataLines = [24, Inf];
        opts.Delimiter = " ";

        % Specify column names and types
        opts.VariableNames = ["Time", "CS", "hF", "QD", "hF2", "QD1", "hE", "QD2", "hEs", "QD3", "VarName11", "VarName12", "VarName13", "VarName14", "VarName15", "VarName16", "VarName17"];
        opts.VariableTypes = ["datetime", "double", "double", "categorical", "double", "categorical", "double", "categorical", "double", "categorical", "datetime", "datetime", "datetime", "datetime", "datetime", "datetime", "datetime"];

        % Specify file level properties
        opts.ExtraColumnsRule = "ignore";
        opts.EmptyLineRule = "read";
        opts.ConsecutiveDelimitersRule = "join";
        opts.LeadingDelimitersRule = "ignore";

        % Specify variable properties
        opts = setvaropts(opts, ["QD", "QD1", "QD2", "QD3"], "EmptyFieldRule", "auto");
        opts = setvaropts(opts, "Time", "InputFormat", 'uuuu-MM-dd''T''HH:mm:ss.SSS''Z''');
        opts = setvaropts(opts, ["hF2", "hE"], "TrimNonNumeric", true);
        opts = setvaropts(opts, ["hF2", "hE"], "ThousandsSeparator", ",");

        % Import the data
        heightgiro = readtable("ionosphere_data_height.txt", opts);
        variables_names = ["hF","hF2", "hE","hEs"];
        data_type = "Height " + variables_names;
        label = "Sum Height " + variables_names + " " + option;
        name = "height_" + variables_names + "_" + option;
        height_relevant_parameters = table();
        for index_type = 1:length(variables_names)
            values = ArrayRelevantParameters(heightgiro.Time, heightgiro.(variables_names(index_type)), data_type(index_type), label(index_type), name(index_type), option);
            height_relevant_parameters = [height_relevant_parameters, table(values, 'VariableNames', variables_names(index_type))];
        end
          end
    function tec_relevant_parameters =  get_data_tec(option)
              arguments
                option {mustBeMember(option,["day","hour"])}
              end
                %% Import data from text file.
        % Script for importing data from the following text file:
        %
        % 

        %% Initialize variables.
        filename = 'ionosphere_data_tec.txt';

        %% Set up the Import Options and import the data
        opts = delimitedTextImportOptions("NumVariables", 4);

        % Specify range and delimiter
        opts.DataLines = [21, Inf];
        opts.Delimiter = " ";

        % Specify column names and types
        opts.VariableNames = ["Time", "CS", "TEC", "QD"];
        opts.VariableTypes = ["datetime", "double", "double", "categorical"];

        % Specify file level properties
        opts.ExtraColumnsRule = "ignore";
        opts.EmptyLineRule = "read";
        opts.ConsecutiveDelimitersRule = "join";
        opts.LeadingDelimitersRule = "ignore";

        % Specify variable properties
        opts = setvaropts(opts, ["QD"], "EmptyFieldRule", "auto");
        opts = setvaropts(opts, "Time", "InputFormat", 'uuuu-MM-dd''T''HH:mm:ss.SSS''Z''');

        % Import the data
        tec_data = readtable(filename, opts);
        vector_time = tec_data.Time;
        vector_data = tec_data.TEC;
        tec_relevant_parameters =  ArrayRelevantParameters(vector_time, vector_data, "Total Electron Content", "Global TEC", "tec", option);
            
        %% Clear temporary variables
       
    end
    function output_object = get_data_iri2016()
                %% Import data from text file
        % Script for importing data from the following text file:
        %
        %    filename: /home/carloscanodomingo/GIT/MATLAB/Lighting/iri2016_85km_ne.txt
        %
        % Auto-generated by MATLAB on 27-May-2021 15:44:55

        %% Set up the Import Options and import the data
        opts = delimitedTextImportOptions("NumVariables", 2);

        % Specify range and delimiter
        opts.DataLines = [1, Inf];
        opts.Delimiter = ",";

        % Specify column names and types
        opts.VariableNames = ["date", "ne_85"];
        opts.VariableTypes = ["datetime", "double"];

        % Specify file level properties
        opts.ExtraColumnsRule = "ignore";
        opts.EmptyLineRule = "read";

        % Specify variable properties
        opts = setvaropts(opts, "date", "InputFormat", "yyyy-MM-dd HH:mm:ss");

        % Import the data
        iri201685kmne = readtable("iri2016_85km_ne_local.txt", opts);
        output_object = ArrayRelevantParameters(iri201685kmne.date, iri201685kmne.ne_85,  "Electron Content 85Km IRI2016 hemos", "Mean Electron Content 85Km hemis", "iri2016_85_ne_hmis", "day");
                      

%% Clear temporary variables
clear opts
    end
    function output_object = get_data_global_temperature()
        %https://data.giss.nasa.gov/gistemp/
        %{
        Global-mean monthly, seasonal, and annual means, 1880-present, updated through most recent month: TXT, CSV
        %}
        tas_mean  = ncread('ionosphere_data_temperature.nc','tas_mean');
        time = ncread('ionosphere_data_temperature.nc','time');
        time_month = arrayfun(@(x) datetime(1850, 1, 1, x * 24, 0, 0 ), time);
        selected = (year(time_month) > 2015 & year(time_month) < 2021);
        time_month_select = time_month(selected); 
        vector_data = tas_mean(selected);
        vector_time = arrayfun(@(date) datetime(year(date), month(date), 1), time_month_select);
        output_object = ArrayRelevantParameters(vector_time, vector_data, "Temperature Anomaly", "Mean Global Anomaly", "diff_temp", "day");
                %% Set up the Import Options and import the data
        opts = delimitedTextImportOptions("NumVariables", 2);

        % Specify range and delimiter
        opts.DataLines = [1, Inf];
        opts.Delimiter = ",";

        % Specify column names and types
        opts.VariableNames = ["time", "global_temp"];
        opts.VariableTypes = ["datetime", "double"];

        % Specify file level properties
        opts.ExtraColumnsRule = "ignore";
        opts.EmptyLineRule = "read";

        % Specify variable properties
        opts = setvaropts(opts, "time", "InputFormat", "dd/MM/yy");

        % Import the data
        tempv2 = readtable("temp_v2.csv", opts);
        output_object = ArrayRelevantParameters(tempv2.time, tempv2.global_temp, "Temperature Anomaly", "Mean Global Anomaly", "diff_temp", "day");
 

    
    
    end
          
          
          function output_object  = get_data_lightning()
                      %% Set up the Import Options and import the data
                opts = delimitedTextImportOptions("NumVariables", 4);

                % Specify range and delimiter
                opts.DataLines = [1, Inf];
                opts.Delimiter = ",";

                % Specify column names and types
                opts.VariableNames = ["ZDAY", "Var2", "Var3", "TOTAL_COUNT"];
                opts.SelectedVariableNames = ["ZDAY", "TOTAL_COUNT"];
                opts.VariableTypes = ["string", "string", "string", "double"];

                % Specify file level properties
                opts.ExtraColumnsRule = "ignore";
                opts.EmptyLineRule = "read";

                % Specify variable properties
                opts = setvaropts(opts, ["ZDAY", "Var2", "Var3"], "WhitespaceRule", "preserve");
                opts = setvaropts(opts, ["ZDAY", "Var2", "Var3"], "EmptyFieldRule", "auto");
                total_time = [];
                total_values = [];
                for index_year=2016:2020
                    % Import the data
                    nldntiles = readtable("nldn-tiles-" + num2str(index_year) + ".csv", opts);
                    %% Convert to output type
                    % Remove text rows
                    nldntiles = nldntiles(4:end, :);

                    %sum by group
                    summary = groupsummary(nldntiles, 'ZDAY', 'sum');

                    %Remove count row
                    summary = summary(:,[1:3]);

                    time = arrayfun(@(a) datetime(a, 'InputFormat',"yyyyMMdd"),table2array(summary(:,1)));
                    data = table2array(summary(:,2));

                    total_time = [total_time, time'];
                    total_values = [total_values, data'];
                end
                output_object = ArrayRelevantParameters(total_time, total_values, "Total Lighting", "Sum Lightning Day", "lightning_day", "day");
          end
          
          function  output_object = get_data_solar_flux()
                              %% Import data from text file.
                % Script for importing data from the following text file:
                %
                %    /home/carloscanodomingo/GIT/MATLAB/Lighting/fluxtable.txt
                %
                % To extend the code to different selected data or a different text file, generate a function instead of a script.
                %{
                # F10.7 Solar Radio Flux
                # Local noon-time observed (F10.7obs) and adjusted (F10.7adj) solar radio flux F10.7 in s.f.u. (10^-22 W m^-2 Hz^-1) is provided by 
                # Dominion Radio Astrophysical Observatory and Natural Resources Canada.
                # Described in: Tapping, K.F., 2013. The 10.7 cm solar radio flux (F10.7). Space Weather, 11, 394-406, https://doi.org/10.1002/swe.20064 
                # Note: For ionospheric and atmospheric studies the use of F10.7obs is recommended.
                %}
                % Auto-generated by MATLAB on 2021/05/22 19:05:56

                %% Initialize variables.
                filename = 'ionosphere_data_solar_flux.txt';
                    %https://spaceweather.gc.ca/forecast-prevision/solar-solaire/solarflux/sx-5-en.php
                %% Read columns of data as text:
                % For more information, see the TEXTSCAN documentation.
                formatSpec = '%8s%*10*s%*17*s%*11s%14s%13s%13s%[^\n\r]';

                %% Open the text file.
                fileID = fopen(filename,'r');

                %% Read columns of data according to the format.
                % This call is based on the structure of the file used to generate this code. If an error occurs for a different file, try regenerating the code from the Import Tool.
                dataArray = textscan(fileID, formatSpec, 'Delimiter', '', 'WhiteSpace', '', 'TextType', 'string',  'ReturnOnError', false);

                %% Close the text file.
                fclose(fileID);

                %% Convert the contents of columns containing numeric text to numbers.
                % Replace non-numeric text with NaN.
                raw = repmat({''},length(dataArray{1}),length(dataArray)-1);
                for col=1:length(dataArray)-1
                    raw(1:length(dataArray{col}),col) = mat2cell(dataArray{col}, ones(length(dataArray{col}), 1));
                end
                numericData = NaN(size(dataArray{1},1),size(dataArray,2));

                for col=[2,3,4]
                    % Converts text in the input cell array to numbers. Replaced non-numeric text with NaN.
                    rawData = dataArray{col};
                    for row=1:size(rawData, 1)
                        % Create a regular expression to detect and remove non-numeric prefixes and suffixes.
                        regexstr = '(?<prefix>.*?)(?<numbers>([-]*(\d+[\,]*)+[\.]{0,1}\d*[eEdD]{0,1}[-+]*\d*[i]{0,1})|([-]*(\d+[\,]*)*[\.]{1,1}\d+[eEdD]{0,1}[-+]*\d*[i]{0,1}))(?<suffix>.*)';
                        try
                            result = regexp(rawData(row), regexstr, 'names');
                            numbers = result.numbers;

                            % Detected commas in non-thousand locations.
                            invalidThousandsSeparator = false;
                            if numbers.contains(',')
                                thousandsRegExp = '^[-/+]*\d+?(\,\d{3})*\.{0,1}\d*$';
                                if isempty(regexp(numbers, thousandsRegExp, 'once'))
                                    numbers = NaN;
                                    invalidThousandsSeparator = true;
                                end
                            end
                            % Convert numeric text to numbers.
                            if ~invalidThousandsSeparator
                                numbers = textscan(char(strrep(numbers, ',', '')), '%f');
                                numericData(row, col) = numbers{1};
                                raw{row, col} = numbers{1};
                            end
                        catch
                            raw{row, col} = rawData{row};
                        end
                    end
                end

                % Convert the contents of columns with dates to MATLAB datetimes using the specified date format.
                try
                    dates{1} = datetime(dataArray{1}, 'Format', 'yyyyMMdd', 'InputFormat', 'yyyyMMdd');
                catch
                    try
                        % Handle dates surrounded by quotes
                        dataArray{1} = cellfun(@(x) x(2:end-1), dataArray{1}, 'UniformOutput', false);
                        dates{1} = datetime(dataArray{1}, 'Format', 'yyyyMMdd', 'InputFormat', 'yyyyMMdd');
                    catch
                        dates{1} = repmat(datetime([NaN NaN NaN]), size(dataArray{1}));
                    end
                end

                dates = dates(:,1);

                %% Split data into numeric and string columns.
                rawNumericColumns = raw(:, [2,3,4]);

                %% Replace non-numeric cells with NaN
                R = cellfun(@(x) ~isnumeric(x) && ~islogical(x),rawNumericColumns); % Find non-numeric cells
                rawNumericColumns(R) = {NaN}; % Replace non-numeric cells

                %% Create output variable
                fluxtable = table;
                vector_time = dates{:, 1};
                fluxtable.ingtonFluxob = ArrayRelevantParameters(vector_time, cell2mat(rawNumericColumns(:,1)), "Flux  Observable", "Mean Flux  Observable", "flux_obv", "day");
                fluxtable.sfluxFluxad = ArrayRelevantParameters(vector_time, cell2mat(rawNumericColumns(:,2)), "Flux  Adj", "Mean Flux  Adjusted", "flux_adj", "day");
                fluxtable.jfluxFluxur = ArrayRelevantParameters(vector_time, cell2mat(rawNumericColumns(:,3)),  "FLux  Series D ", "Mean Flux  serie D", "flux_ser_d", "day");
                output_object = fluxtable;
                
                     
          end
          function save_all(relevant, sr_day, magnitude)
           arguments
                relevant 
                sr_day (1,1) SR_day_array
                magnitude {mustBeMember(magnitude,["f","b"])}
           end
           if istable(relevant)
               for index_table = 1:width(relevant)
                    current_object = relevant{1,index_table};
                    current_object.save_correlation(sr_day, magnitude);
               end
           else
               relevant.save_correlation(sr_day);
           end
               
          end
    end
end

