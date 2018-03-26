
	
tp = 0; tn = 0; fp = 0; fn = 0;

N=100;

for i = 1:N
	switch_event = round(rand) * 2;
	[t x] = generator(switch_event); 

%	plot(t,x)

	y = detector(x);

	if (switch_event > 0) 
		if (y) 
			tp++;
		else
			fp++;
		end
	else
		if (y) 
			fn++;
		else
			tn++;
		end
	end
end

tp
tn
fp
fn

