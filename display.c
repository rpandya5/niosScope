#include <stdlib.h>

#include <stdint.h>

#define ADC_BASE 0xFF204000
#define LEDs_BASE 0xFF200000
#define GPIO_BASE 0xFF200070
#define BUFFER_SIZE 1024

volatile unsigned int adc_buffer[BUFFER_SIZE];
volatile int write_index = 0;
volatile int read_index = 0;
unsigned int read_index_graphics = 0;

unsigned int read_adc(void);
void buffer_add(unsigned int sample);
unsigned int buffer_get(void);

volatile int pixel_buffer_start; // global variable
short int Buffer1[240][512]; // 240 rows, 512 (320 + padding) columns
short int Buffer2[240][512];

int temp[16] = {0};

#define XMAX 320
#define YMAX 240

int murder_me[XMAX] = {0};

#define STEP_SIZE XMAX / BUFFER_SIZE

#define GRAY 0x0100105
#define BLUE 0x0000FF
#define RED 0xFF0000
#define WHITE 0xFFFFFF
	
int main(void){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    // declare other variables(not shown)
    // initialize location and direction of rectangles(not shown)
    /* set front pixel buffer to Buffer 1 */
    *(pixel_ctrl_ptr + 1) = (int) &Buffer1; // first store the address in the  back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer

    /* set back pixel buffer to Buffer 2 */
    *(pixel_ctrl_ptr + 1) = (int) &Buffer2;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
    clear_screen(); // pixel_buffer_start points to the pixel buffer
	int points[2] = {-1,0};

    while (1)
    {
        // code for drawing the boxes and lines (not shown)
        // code for updating the locations of boxes (not shown)
		update(&points);
		murder();
		background();
		draw(&squares);

        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}



unsigned int read_adc(void) {
  volatile unsigned int* ADC_control = (unsigned int*)ADC_BASE;
  volatile unsigned int* LEDs_control = (unsigned int*)LEDs_BASE;
  volatile unsigned int* GPIO_control = (unsigned int*)GPIO_BASE;

  *ADC_control = 1;
  unsigned int result = *ADC_control & 0xFFF;

  *LEDs_control = result;

  for (volatile int delay = 0; delay < 1000; delay++);
  return result;
}

void buffer_add(unsigned int sample) {  // WRITE NEXT SAMPLE
  adc_buffer[write_index] = sample;

  write_index++;
  if (write_index >= BUFFER_SIZE) write_index = 0;

  // IF FULL OVERWRITE OLDEST SAMPLE
  if (write_index == read_index) {
    read_index++;
    if (read_index >= BUFFER_SIZE) read_index = 0;
  }
}

unsigned int buffer_get(void) {  // READ SAMPLE
  unsigned int sample = adc_buffer[read_index];

  read_index++;
  if (read_index_graphics >= BUFFER_SIZE) read_index = 0;

  return sample;
}


unsigned int graphic_buffer_get(void) {  // READ SAMPLE
  unsigned int sample = adc_buffer[STEP_SIZE*read_index_graphics];

  read_index_graphics+=STEP_SIZE;
  if (read_index_graphics >= BUFFER_SIZE) read_index_graphics = 0;

  return sample;
}





// code for subroutines (not shown)


void wait_for_vsync(){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	*pixel_ctrl_ptr = 1;
	while((*(pixel_ctrl_ptr+3))&0x1){}

}

void plot_pixel(int x, int y, short int line_color)
{
    volatile short int *one_pixel_address;
        
        one_pixel_address = pixel_buffer_start + (y << 10) + (x << 1);
        
        *one_pixel_address = line_color;
}

void clear_screen(){
	for (int i=0; i<XMAX; i++){
		for(int j=0; j<YMAX; j++){
			plot_pixel(i,j,0);	
		}
	}
	
}

void draw_line(int x0, int y0, int x1, int y1, short int line_color){
	int deltaX = x1 - x0;
	int deltaY = y1 - y0;
	
	
	
	if ((deltaX == 0) && (deltaY == 0)){plot_pixel(x0, y0, line_color);}
	else if (abs(deltaX) >= abs(deltaY)){
		if (deltaX > 0){
			for (int i = 0; i<= deltaX; i++){
				plot_pixel(x0+i, (deltaY*(i))/ deltaX+y0, line_color);
			}
		} else{
			for (int i = deltaX; i< 0; i++){
				plot_pixel(x0+i, (deltaY*(i))/deltaX+y0, line_color);
			}
		}
	}
	else{
		if (deltaY > 0){
			for (int i = 0; i< deltaY; i++){
				plot_pixel((deltaX*(i))/ deltaY+x0, y0+i, line_color);
			}
		} else{
			for (int i = deltaY; i< 0; i++){
				plot_pixel((deltaX*(i))/ deltaY+x0, y0+i, line_color);

			}
		}
	}
}


void background(){
	draw_line(0, YMAX/2, XMAX-1, YMAX/2, GRAY);
	draw_line(0, YMAX/2+1, XMAX-1, YMAX/2+1, GRAY);
	draw_line(0, YMAX/2-1, XMAX-1, YMAX/2-1, GRAY);
	
	draw_line(XMAX/2, 0, XMAX/2, YMAX-1, GRAY);
	draw_line(XMAX/2+1, 0, XMAX/2+1, YMAX-1, GRAY);
	draw_line(XMAX/2-1, 0, XMAX/2-1, YMAX-1, GRAY);
	
	for (int i = 0; i < XMAX-1; i=i+10){
		draw_line(i, 0, i, YMAX-1, GRAY);
	}
	for (int i = 0; i < YMAX-1; i=i+10){
		draw_line(0, i, XMAX-1, i, GRAY);
	}
}

void update(int* change_me){
	for (int i = 0; i< XMAX; i++){
		if (i == 0){
			change_me[0] = graphic_buffer_get(); //ADD NORMALIZER
		} else{
			change_me[1] = change_me[0];
			change_me[0] = graphic_buffer_get();
			draw_line(i-1, change_me[0], i, change_me[1], WHITE);
		}
	
	
	
	}
	if (change_me[0] == 0 && change_me[1] == 0){
		change_me[1] = graphic_buffer_get(); //ADD NORMALIZER
	}else{
		change_me[3] == change_me[1];
		change_me[2] == change_me[0];
		change_me[1] = buffer_get();
		change_me[0] = change_me[0]+1;
		
	
	
	}
	
	murder_me[i*2] = temp[i*2]; 
	murder_me[i*2+1] = temp[i*2+1];
	}

	}

void murder(){
	
	for (int j = 0; j<8; j++){
		for (int i = -2; i < 3; i++){
			for (int k = -2; k < 3; k++){
				plot_pixel(*(murder_me+j*2)+i,*(murder_me+j*2+1)+k,0);	
			}
		}
		if(j<7){
			draw_line(*(murder_me+j*2), *(murder_me+j*2+1), *(murder_me+(j+1)*2), *(murder_me+(j+1)*2+1), 0);
		}else{
			draw_line(*(murder_me+j*2), *(murder_me+j*2+1), *(murder_me), *(murder_me+1), 0);
		}
	}
}

void draw(int*draw_me){	
	for (int i = 0; i< 8; i++){

		
		for (int j = -2; j < 3; j++){
			for (int k = -2; k < 3; k++){
				plot_pixel(*(draw_me+i*5)+j,*(draw_me+i*5+1)+k,*(draw_me+i*5+2));	
			}
		}
		
		
		if(i<7){
			draw_line(*(draw_me+i*5), *(draw_me+i*5+1), *(draw_me+(i+1)*5), *(draw_me+(i+1)*5+1), *(draw_me+i*5+2));
		} else{
			draw_line(*(draw_me+i*5), *(draw_me+i*5+1), *(draw_me), *(draw_me+1), *(draw_me+i*5+2));
		}
	}


}
