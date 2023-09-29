/**
* (This program applys filters to bmp files using threads)
*
* Completion time: (estimation of hours spent on this program)
*
* @author (Ethan Wilson), (Ruben Acuna)
* @version (1.0)
*/

////////////////////////////////////////////////////////////////////////////////
//INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "BmpProcessor.h"
////////////////////////////////////////////////////////////////////////////////
//MACRO DEFINITIONS

//problem assumptions
#define BMP_HEADER_SIZE 14
#define BMP_DIB_HEADER_SIZE 40
#define MAXIMUM_IMAGE_SIZE 4096

#define THREAD_COUNT 4
#define BOX_BLUR_RADIUS 3

////////////////////////////////////////////////////////////////////////////////
//DATA STRUCTURES

struct BMP_Header* bmp_header;
struct DIB_Header* dib_header;
struct Pixel** pArr;

struct thread_data {
	struct Pixel **pArr;
	int width, height, startY, endY;
};


////////////////////////////////////////////////////////////////////////////////
//MAIN PROGRAM CODE

struct Pixel blurPixel(struct thread_data* threadData, int x, int y){
	int sumR = 0, sumG = 0, sumB = 0, count = 0;

	for(int i = -BOX_BLUR_RADIUS; i <= BOX_BLUR_RADIUS; i++){
		for(int j = -BOX_BLUR_RADIUS; j <= BOX_BLUR_RADIUS; j++){
			int newX = x + i;
			int newY = y + j;
			if (newX >= 0 && newX < threadData->width && newY >= 0 && newY < threadData->height){
				sumR += threadData->pArr[newY][newX].red;
				sumG += threadData->pArr[newY][newX].green;
				sumB += threadData->pArr[newY][newX].blue;
				count++;
			}
		}
	}
	struct Pixel blurred;
	blurred.red = (unsigned char)(sumR / count);
	blurred.green = (unsigned char)(sumG / count);
	blurred.blue = (unsigned char)(sumB / count);
	return blurred;
}

void* boxBlurThread(void* args){
	struct thread_data* threadData = (struct thread_data*) args;
	for(int i = threadData->startY; i < threadData->endY; i++){
		for(int j = 0; j < threadData->width; j++){
			threadData->pArr[i][j] = blurPixel(threadData, j, i);
		}
	}
	return NULL;
}

void applyBoxBlurFilter(struct Pixel** pArr, int width, int height){
	pthread_t threads[THREAD_COUNT];
	struct thread_data threadData[THREAD_COUNT];

	int rowsPerThread = height / THREAD_COUNT;

	for (int i = 0; i < THREAD_COUNT; i++) {
		threadData[i].pArr = pArr;
		threadData[i].width = width;
		threadData[i].height = height;
		threadData[i].startY = i * rowsPerThread;
		threadData[i].endY = (i == THREAD_COUNT - 1) ? height : (i + 1) * rowsPerThread;

		pthread_create(&threads[i], NULL, boxBlurThread, (void*)&threadData[i]);
	}

	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join(threads[i], NULL);
	}
}

void* swissCheeseThread(void* args){
	struct thread_data* threadData = (struct thread_data *) args;
	struct Pixel** pArr = threadData->pArr;
	int width = threadData->width;
	int height = threadData->height;

	//int smallestSide = (width < height) ? width : height;
	//int numHoles = smallestSide * 8 / 100;
        int numHoles = (width < height) ? width / 10 : height / 10 / 2;
        int averageRadius = (width < height) ? width / 10 : height / 10;

        for (int i = 0; i < numHoles; i++){
                //m Calculate random values for height width and radius
                int centerX = rand() % width;
                int centerY = rand() % height;
                int radius = rand() % (averageRadius / 2) + (averageRadius / 2);

                // Ensure tbe holes stay in the bounds of the pixel array
                if (centerX - radius >= 0 && centerX + radius < width && centerY - radius >= 0 && centerY + radius < height){
                        for (int y = centerY - radius; y <= centerY + radius; y++){ // Fill in the circles (holes)
                                for (int x = centerX - radius; x <= centerX + radius; x++){
                                        int dx = x - centerX;
                                        int dy = y - centerY;
                                        if (dx * dx + dy * dy <= radius * radius){
                                                pArr[y][x].red = 0;
                                                pArr[y][x].green = 0;
                                                pArr[y][x].blue = 0;
                                        }
                                }
                        }
                }
        }
}

void blackAndWhite(struct Pixel** pArr, int width, int height) {

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			unsigned char grayscale = (unsigned char)(0.299 * pArr[i][j].red + 0.587 * pArr[i][j].green + 0.114 * pArr[i][j].blue);
			pArr[i][j].red = grayscale;
			pArr[i][j].green = grayscale;
			pArr[i][j].blue = grayscale;
		}
	}
}

void applySwissCheeseFilter(struct Pixel** pArr, int width, int height){

	//Make image black and white first
	blackAndWhite(pArr, width, height);

        // Apply the yellow tint
        for(int i = 0; i < height; i++){
                for(int j = 0; j < width; j++){
			int shiftR, shiftG, shiftB;
			shiftR = pArr[i][j].red + 50;
                        shiftG = pArr[i][j].green + 50 ;
                        shiftB = pArr[i][j].blue - 30;
			if(shiftR > 255) shiftR = 255;
			if(shiftB > 255) shiftB = 255;
			if(shiftG > 255) shiftG = 255;
                        pArr[i][j].red = shiftR;
                        pArr[i][j].green = shiftG;
                        pArr[i][j].blue = shiftB;
                }
        }

	pthread_t threads[THREAD_COUNT];
	struct thread_data threadData[THREAD_COUNT];
	int rowsPerThread = height / THREAD_COUNT;

	for(int i = 0; i < THREAD_COUNT; i++){
                threadData[i].pArr = pArr;
                threadData[i].width = width;
                threadData[i].height = height;
                threadData[i].startY = i * rowsPerThread;
                threadData[i].endY = (i == THREAD_COUNT - 1) ? height : (i + 1) * rowsPerThread;

                pthread_create(&threads[i], NULL, swissCheeseThread, (void*)&threadData[i]);
	}

        for (int i = 0; i < THREAD_COUNT; i++) {
                pthread_join(threads[i], NULL);
        }
}

// My own reading and writing pixel functions
void wilsonReadPixelsBMP(FILE* file, struct Pixel** pArr, int width, int height){
	for(int i = 0; i < height; i++){
		pArr[i] = (struct Pixel*)malloc(width * sizeof(struct Pixel));
	}
	for(int i = 0; i < height; i++){
		for (int j = 0; j < width; j++){
			fread(&pArr[i][j].blue, sizeof(unsigned char), 1, file);
			fread(&pArr[i][j].green, sizeof(unsigned char), 1, file);
			fread(&pArr[i][j].red, sizeof(unsigned char), 1, file);
		}
		// Calculate padding
		int padding = (int) (4 - (width * sizeof(struct Pixel)) % 4) % 4;
		for (int p = 0; p < padding; p++){
			fgetc(file); // Read padding and discard it
		}
	}
}

void wilsonWritePixelsBMP(FILE* file, struct Pixel** pArr, int width, int height){
	int padding = (int) (4 - (width * sizeof(struct Pixel)) % 4) % 4;

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			fwrite(&pArr[i][j].blue, sizeof(unsigned char), 1, file);
			fwrite(&pArr[i][j].green, sizeof(unsigned char), 1, file);
			fwrite(&pArr[i][j].red, sizeof(unsigned char), 1, file);
		}
		unsigned char paddingByte = 0;
		for (int p = 0; p < padding; p++) {
			fwrite(&paddingByte, sizeof(unsigned char), 1, file);
		}
	}
}

void destroyImage(struct Pixel** pArr, int height){
	for (int i = 0; i < height; i++) {
		free(pArr[i]);
	}
	free(pArr);
	pArr = NULL;
}

void main(int argc, char* argv[]) {
        // Arg structure
        // argv[1] = i, argv[2] = i_filename, argv[3] = o, argv[4] = o_filename, argv[5] = f, argv[6] = b || c
	char* input_filename = argv[2];
	char* output_filename = argv[4];
	char* filter = argv[6];
	FILE* inFile = fopen(input_filename, "rb");
	FILE* outFile = fopen(output_filename, "wb");
	if(!inFile){
		perror("Error opening read file");
		exit(1);
	}
        if(!outFile){
                perror("Error opening write file");
                exit(1);
        }
	bmp_header = (struct BMP_Header *) malloc(BMP_HEADER_SIZE);
	dib_header = (struct DIB_Header *) malloc(BMP_DIB_HEADER_SIZE);
	readBMPHeader(inFile, bmp_header);
	readDIBHeader(inFile, dib_header);
	int width = dib_header->width, height = dib_header->height;

	pArr = (struct Pixel**) malloc(dib_header->height * sizeof(struct Pixel**));
	wilsonReadPixelsBMP(inFile, pArr, width, height);

	pthread_t threads[THREAD_COUNT];
	if(strcmp(filter, "b") == 0){
		applyBoxBlurFilter(pArr, width, height);
		writeBMPHeader(outFile, bmp_header);
		writeDIBHeader(outFile, dib_header);
		wilsonWritePixelsBMP(outFile, pArr, width, height);

	} else if(strcmp(filter, "c") == 0){
                applySwissCheeseFilter(pArr, width, height);
                writeBMPHeader(outFile, bmp_header);
                writeDIBHeader(outFile, dib_header);
                wilsonWritePixelsBMP(outFile, pArr, width, height);
	}
	fclose(inFile);
	fclose(outFile);
	destroyImage(pArr, dib_header->height);
}
