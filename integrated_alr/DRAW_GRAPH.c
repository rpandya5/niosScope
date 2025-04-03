/*
APril 1st, 9am, this is the code to convert raw adc or trigger arrays into a drawn graph


*/


void draw_graph(int samples[], float amplitude, int freq){
	  // CLEAR THE FINAL_WAVE ARRAY BEFORE PLOTTING NEW DATA
	  for (int i = 0; i < XMAX; i++) {
		final_wave[i] = 0;
	  }
	float voltageFloat;
	for (int i=0; i < 80; i++){
		//for (int j = 0; j<SAMPLE_RATE; j++){}
		voltageFloat = samples[(index+i)%80] * (1.0 / 4095.0);
		plot_shifted_sinc(i*4-XMAX/2, voltageFloat);
		
	}
	background();
	for(int i=0; i<XMAX-1; i++){
		draw_line(i, YMAX/2 - delete_wave[i], i+1, YMAX/2 - delete_wave[i+1], BLACK);
		draw_line(i, YMAX/2 - final_wave[i], i+1, YMAX/2 - final_wave[i+1], WHITE);
		delete_wave[i] = final_wave[i];
		final_wave[i] = 0;

	}
		delete_wave[XMAX-1] = final_wave[XMAX-1];
		final_wave[XMAX-1] = 0;
        display_freq(0);
        display_amplitude(0.0);
}
