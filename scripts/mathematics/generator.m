function [t x] = generator(switch_event=0,cycles=3)

	samples=100;
	resolution=2*pi/samples;
	t=0:resolution:2*pi*cycles-resolution;
	center_line=127.5;

	x=round((1-cos(t))*center_line);

	if (switch_event>0) 
		switch_delay = 25;
		start = round(rand * (samples * cycles - switch_delay - 1)) + 1;
		if (switch_event == 1)
			for i=1:switch_delay
				x(start+i) = x(start);
			end
		else
			for i=1:switch_delay
				x(start+i) = ( x(start+i-1) - center_line) * 80/100 + center_line;
			end
		end
	end
end
