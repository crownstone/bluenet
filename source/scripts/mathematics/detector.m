
function [result] = detector(data)
	result = false;		

	D=length(data);

	K = 6;
	d = D/K;

	zero_crossing=mean(data);

	subdata = reshape(data, d, K)';

	% not used
	% make symmetrical
%	for i=1:K
%		subdata(i,:) = zero_crossing + abs( subdata(i,:) - zero_crossing );
%	end

	checkdata = [];
	for i=1+1:K-1
		checkdata = [checkdata, subdata(i,:)];
	end

	dc=diff(checkdata);

	% just a consecutive series of differential values that are close to zero and which series is long enough
	consecutive_min = 10;
	consecutive_threshold = 10;
	for i=1:length(dc)-consecutive_min
		zerosdetected = mean(abs(dc(1+i:consecutive_min+i)));
		% a lot of zeros (should be under a threshold level)
		if (zerosdetected < consecutive_threshold)
			result = true;
			break;
		end
	end

	% it seems this detects both types of events just fine!	

	% not used
	% data_mirrored=reshape(subdata', 1, D);

end
