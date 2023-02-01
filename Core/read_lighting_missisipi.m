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
ArrayRelevantParameters.create_array(total_time, total_values);
hold on
%% Clear temporary variables