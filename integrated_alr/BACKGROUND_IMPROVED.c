#include <stdlib.h>
#include <stdio.h>
#include <math.h>


volatile int* LEDs = 0xFF200000;
	
volatile int pixel_buffer_start; // global variable


#define XMAX 320
#define YMAX 240

#define GRAY 0x1501314
#define BLUE 0x0000FF
#define RED 0xFF0000
#define WHITE 0xFFFFFF
#define BLACK 0

void plot_pixel(int x, int y, int line_color);
void plot_point(int x, int y, int line_color);
void draw_line(int x0, int y0, int x1, int y1, int line_color);
void background();

// Define the 7 segments (A-G) for a hex display
int segments[7][4] = {
    {0, 0, 5, 0}, // Segment A (Top)
    {5, 0, 5, 4}, // Segment B (Upper right)
    {5, 4, 5, 8}, // Segment C (Lower right)
    {0, 8, 5,8}, // Segment D (Bottom)
    {0, 8, 0, 4}, // Segment E (Lower left)
    {0, 4, 0, 0}, // Segment F (Upper left)
    {0, 4, 5, 4}  // Segment G (Middle)
};

// Define which segments are used for each digit (0-9)
int digit_segments[10][7] = {
    {1, 1, 1, 1, 1, 1, 0}, // Digit 0 (Segments A, B, C, D, E, F)
    {0, 1, 1, 0, 0, 0, 0}, // Digit 1 (Segments B, C)
    {1, 1, 0, 1, 1, 0, 1}, // Digit 2 (Segments A, B, D, E, G)
    {1, 1, 1, 1, 0, 0, 1}, // Digit 3 (Segments A, B, C, D, G)
    {0, 1, 1, 0, 0, 1, 1}, // Digit 4 (Segments B, C, F, G)
    {1, 0, 1, 1, 0, 1, 1}, // Digit 5 (Segments A, C, D, F, G)
    {1, 0, 1, 1, 1, 1, 1}, // Digit 6 (Segments A, C, D, E, F, G)
    {1, 1, 1, 0, 0, 0, 0}, // Digit 7 (Segments A, B, C)
    {1, 1, 1, 1, 1, 1, 1}, // Digit 8 (All segments)
    {1, 1, 1, 1, 0, 1, 1}  // Digit 9 (Segments A, B, C, D, F, G)
};




int main(void){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    pixel_buffer_start = *(pixel_ctrl_ptr); // we draw on the back buffer
    clear_screen(); // pixel_buffer_start points to the pixel buffer
	background();
	
    while (1)
    {
    	}
	return 0;
}

// code for subroutines (not shown)

void plot_pixel(int x, int y, int line_color)
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

void draw_line(int x0, int y0, int x1, int y1, int line_color){
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


void plot_point(int x, int y, int line_color){
	if (abs(y)<YMAX/2){
		plot_pixel(x, YMAX/2-y, line_color);
	}

}


void display_value(int value, int x, int y, int colour){	
		for (int i =0; i<7; i++){
			if(digit_segments[value][i]){
				draw_line(segments[i][0]+x,segments[i][1]+y,segments[i][2]+x,segments[i][3]+y, colour);
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
	
	for (int i = 0; i < XMAX-1; i=i+16){
		draw_line(i, 0, i, YMAX-1, GRAY);
	}
	for (int i = 0; i < YMAX-1; i=i+12){
		draw_line(0, i, XMAX-1, i, GRAY);
	}
	//drawing info box
	int BOX_THICKNESS = 12;
	for (int i = 0; i < BOX_THICKNESS; i++){
		draw_line(0, YMAX-1-i, XMAX-1, YMAX-1-i, GRAY);
	
	}
	int amp_points[][2] = {
    // A
    {0, 0}, {1, 0}, {2, 0}, {3, 0}, {0, 1}, {3, 1}, {0, 2}, {3, 2}, {0, 3}, {1, 3}, {2, 3}, {3, 3}, {0, 4}, {3, 4}, {0, 5}, {3, 5},
    {5, 0}, {6, 0}, {7, 0}, {8, 0}, {5, 1}, {7, 1}, {5, 2}, {7, 2}, {5, 3}, {7, 3}, {5, 4}, {7, 4}, {5, 5}, {7, 5}, {9, 1}, {9, 2}, {9, 3}, {9, 4}, {9, 5},
    // p
    {11, 0}, {12, 0}, {13, 0}, {14, 0}, {11, 1}, {14, 1}, {11, 2}, {14, 2}, {11, 3}, {12, 3}, {13, 3}, {11, 4}, {11, 5},
    // =
    {15, 2}, {16, 2}, {17, 2}, {18, 2}, {15, 4}, {16, 4}, {17, 4}, {18, 4}
	};
	int freq_points[][2] = {
    {0, 0}, {1, 0}, {2, 0}, {3, 0}, {0, 1}, {0, 2}, {1, 2}, {2, 2}, {0, 3}, {0, 4}, {0, 5}, // F
    {5, 0}, {6, 0}, {7, 0}, {5, 1}, {7, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 5}, // r
    {10, 0}, {11, 0}, {12, 0}, {13, 0}, {10, 1}, {10, 2}, {11, 2}, {12, 2}, {10, 3}, {10, 4}, {11, 4}, {12, 4}, {13, 4}, {10, 5}, {11, 5}, {12, 5}, {13, 5},
    {15, 0}, {16, 0}, {17, 0}, {18, 0}, {15, 1}, {18, 1}, {15, 2}, {18, 2}, {15, 3}, {16, 3}, {17, 3}, {18, 3}, {18, 4}, {18, 5}, // q
    {20, 2}, {21, 2}, {22, 2}, {23, 2}, {20, 4}, {21, 4}, {22, 4}, {23, 4} // =
	};
	int amp_size = sizeof(amp_points) / sizeof(amp_points[0]);
	int freq_size = sizeof(freq_points) / sizeof(freq_points[0]);

	for (int i = 0; i<amp_size; i++){
		plot_pixel(amp_points[i][0],amp_points[i][1]+YMAX-BOX_THICKNESS+2, WHITE);
	}
	for (int i = 0; i<freq_size; i++){
		plot_pixel(freq_points[i][0]+60,freq_points[i][1]+YMAX-BOX_THICKNESS+2, WHITE);
	}
	
}
