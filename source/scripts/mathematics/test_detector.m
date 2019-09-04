#!/bin/octave
	
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
			fn++;
		end
	else
		if (y) 
			fp++;
		else
			tn++;
		end
	end
end

tp
tn
fp
fn

printf("\nWith a false positive the detector detects a switch event, \n   - but it did not actually happen.\n");
printf("With a false negative the detector detects nothing, \n   - but there was actually a switch event.\n\n");
printf("We do not want to have lights suddenly turn on by accident. Hence, false positives are really bad.\n");
printf("Of course also false negatives should be really low as well.\n\n");
