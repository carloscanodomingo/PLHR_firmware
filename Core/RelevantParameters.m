classdef RelevantParameters
    %RELEVANTPARAMETERS Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        date;
        std;
        mean;
        max;
        min
        skewness;
        kurtosis;
    end
    
    methods
        function obj = RelevantParameters(time_vector,data_vector)
            arguments
                time_vector (1,:) datetime
                data_vector (1,:) double
            end
            if ~isequal(size(time_vector),size(data_vector))
                eid = 'Size:notEqual';
                msg = 'Size of first input must equal size of second input.';
                throwAsCaller(MException(eid,msg))
            end
            
            if range(month(time_vector)) ~= 0 || range(year(time_vector)) ~= 0
                eid = 'RelevantParameters:NoSameMonthYear';
                msg = 'All time vector must be the of the same month and year.';
                throwAsCaller(MException(eid,msg))
            end
            obj.date = datetime(year(time_vector(1)), month(time_vector(1)), 1); 
            obj.std = std(data_vector);
            obj.mean = mean(data_vector);
            obj.max = max(data_vector);
            obj.min = min(data_vector);
            obj.skewness = moment(data_vector, 3);
            obj.kurtosis = moment(data_vector, 4);

        end
        
    end
end

