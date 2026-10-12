build:
	gcc -ggdb main.c SR_platform_win32.c SR_matrix.c SR_renderer.c -o sr -lGdi32