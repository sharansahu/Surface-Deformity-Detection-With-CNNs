#include <stdio.h>   /* printf */
#include <stdlib.h>  /* atoi */
#include <string>    /* string class */
#include <random>    /* random_device, mt19937, uniform_int_distribution */
#include <time.h>    /* clock */
#include <math.h>    /* M_PI define, sqrt */
      
using namespace std;

const int bytesPerPixel = 3; // red, green, blue
const int fileHeaderSize = 14;
const int infoHeaderSize = 40;

#define MIN_DEFORMITY_RADIUS 1
#define MAX_NONE_RATIO 0.05
#define MAX_LOW_RATIO 0.40
#define MAX_MEDIUM_RATIO 0.60
#define MAX_HIGH_RATIO 1

#define CLASSIFICATION_FILENAME "classification.txt"

void generateBitmapImage(unsigned char *image, int height, int width, int pitch, const char* imageFileName);
unsigned char* createBitmapFileHeader(int height, int width, int pitch, int paddingSize);
unsigned char* createBitmapInfoHeader(int height, int width);
void addDeformity(unsigned char *image, int height, int width, int pitch, int maxRadius, string fname);

void printUsage(char* progName)
{
    printf("USAGE: %s <image height> <image width> <number of images to create>", progName);
    printf(" <maximum deformity radius>\n");
    return;
}

int calcMinLowRadius(int height, int width)
{
   return (sqrt(((height * width) * (MAX_NONE_RATIO + 0.01))/M_PI)); 
}

int calcMinMediumRadius(int height, int width)
{
   return (sqrt(((height * width) * (MAX_LOW_RATIO + 0.01))/M_PI)); 
}

int calcMinHighRadius(int height, int width)
{
   return (sqrt(((height * width) * (MAX_MEDIUM_RATIO + 0.01))/M_PI)); 
}

void writeClassificationFile(FILE *classFile, string fileName, int height, int width, unsigned int radius)
{
    unsigned int lowRadiusBoundary = calcMinLowRadius(height, width);
    unsigned int medRadiusBoundary = calcMinMediumRadius(height, width);
    unsigned int highRadiusBoundary = calcMinHighRadius(height, width);
    string classification = "";

    if(radius < lowRadiusBoundary)
    {
       classification  = ", None\n";
    }

    else if(radius >= lowRadiusBoundary && radius < medRadiusBoundary)
    {
       classification  = ", Low\n";
    }

    else if(radius >= medRadiusBoundary && radius < highRadiusBoundary)
    {
       classification  = ", Medium\n";
    }

    else if(radius >= highRadiusBoundary)
    {
       classification  = ", High\n";
    }

    else
    {
       // This shouldn't be possible, but is captured for completeness of code
       printf("ERROR: Radius outside of classification parameters!\n");
    }

#if defined(DEBUG)
    printf("Image %s, Radius = %u, Min Low R = %u, Min Med R = %u, Min High R = %u!\n",
          fileName.c_str(), radius,
          lowRadiusBoundary, medRadiusBoundary, highRadiusBoundary);
#endif
    fwrite(fileName.c_str(), 1, fileName.size(), classFile);
    fwrite(classification.c_str(), 1, classification.size(), classFile);
}

/***
 * main
 * Command Line Parameters -
 *   image height
 *   image width
 *   number of images to create
 *   max deformity radius
 **/
int main(int argc, char* argv[])
{
    if(argc < 4)
    {
        printf("ERROR: invalid number of command line parameters\n");
        printUsage(argv[0]);
        return -1;
    }
    int height = atoi(argv[1]);
    int width = atoi(argv[2]);
    int numOfImages = atoi(argv[3]);
    int maxRadius = atoi(argv[4]);

    if((maxRadius > height/2) || (maxRadius > width/2))
    {
        printf("ERROR: Max radius too large. Must be less than half the width and height\n");
        printUsage(argv[0]);
        return -1;
    }

    clock_t tStart = clock();
    int pitch = width*bytesPerPixel;
    unsigned char image[height][width][bytesPerPixel];
    string imageFileName;
    string classFileName = CLASSIFICATION_FILENAME;
    FILE* classFile = fopen(classFileName.c_str(), "a");

    // Set up the random number generator for the radius and deformity center
    // Obtain a random number from hardware
    std::random_device rd;
    // Seed the generator
    std::mt19937 eng(rd());

    // Define the range the length of the radius can be in (0 means no deformity)
    std::uniform_int_distribution<> radiusDistr(MIN_DEFORMITY_RADIUS, maxRadius);

    int pixelRow, pixelCol, imageNum;
    for(imageNum = 1; imageNum <= numOfImages; imageNum++)
    {
        imageFileName = "bitmap" + std::to_string(imageNum) + ".bmp";
#if !defined(TESTING)
        // Choose the random length of the radius
        unsigned int radius = radiusDistr(eng);
        // Define the columns the deformity center can be in 
        /* 
         * NOTE: Since the ranges for the center of the deformity are dependent on the radius
         * to keep the deformity completely within the image, those ranges must be determined
         * after the radius has been chosen.
         * NOTE: The range has to be one less than the boundary minus the radius to keep the
         * deformity from touching the edge of the image.
         */
        std::uniform_int_distribution<> colDistr(radius+1, width-radius-1);
        // Choose the random column for the center of the deformity 
        unsigned int deformityCenterCol = colDistr(eng);
        // Define the rows the deformity center can be in 
        std::uniform_int_distribution<> rowDistr(radius+1, height-radius-1);
        // Choose the random row for the center of the deformity 
        unsigned int deformityCenterRow = rowDistr(eng);
#else
        // Fixed coordinates for testing
        unsigned int radius = 20;
        unsigned int deformityCenterRow = 21;
        unsigned int deformityCenterCol = 21;
#endif

        printf("Deformity location [%u, %u], radius = %u\n", 
               deformityCenterRow, deformityCenterCol, radius);

        for(pixelRow=0; pixelRow<height; pixelRow++)
        {
            for(pixelCol=0; pixelCol<width; pixelCol++)
            {
#if defined(SQUARE_DEFORMITY)
                /*
                 * For a square deformity, just see if the current "pixel" is within a "radius"
                 * of the center.
                 */
                if(((pixelRow>(deformityCenterRow-radius)) &&
                    (pixelRow<(deformityCenterRow+radius))) &&
                   ((pixelCol>(deformityCenterCol-radius)) &&
                    (pixelCol<(deformityCenterCol+radius))))
#else
                /*
                 * It is necessary to use the Pythagorean Theorem for a circular deformity.
                 * First, calculate the number of rows and the number of columns away from the 
                 * the deformity center that the pixel is. Sum the squares of both of these numbers.
                 * Then, take the square root. If the result is less than the value of the radius,
                 * the pixel is within the bounds of the deformity. Note that absolute values must be
                 * used where the pixel's coordinates are of lower value than the center.
                 */
                unsigned int pixelRowDist = ((pixelRow<deformityCenterRow)?
                                             (deformityCenterRow - pixelRow):
                                             (pixelRow - deformityCenterRow)); 
                unsigned int pixelColDist = ((pixelCol<deformityCenterCol)?
                                             (deformityCenterCol - pixelCol):
                                             (pixelCol - deformityCenterCol)); 
                unsigned int pixelSumOfSquares = pixelRowDist*pixelRowDist +
                                                 pixelColDist*pixelColDist;
                unsigned int radiusSquare = radius*radius;
#if defined(DEBUG)
                if(pixelSumOfSquares < radiusSquare)
                {
                   printf("Pixel [%u,%u] is %u rows and %u columns from Dcenter at [%u,%u]\n",
                          pixelRow, pixelCol,
                          pixelRowDist, pixelColDist,
                          deformityCenterRow, deformityCenterCol);
                   printf("A^2 + B^2 = %u, radius^2 = %u\n", pixelSumOfSquares, radiusSquare);
                }
#endif
                if(pixelSumOfSquares < radiusSquare)
#endif
                {
#if defined(DEBUG)
                    printf("Setting black at [%u, %u]\n", pixelRow, pixelCol); 
#endif
                    // Set to black to represent deformity
                    image[pixelRow][pixelCol][2] = (unsigned char)(0); ///red
                    image[pixelRow][pixelCol][1] = (unsigned char)(0); ///green
                    image[pixelRow][pixelCol][0] = (unsigned char)(0); ///blue
                }
                else
                {
                    // Set to white as the regular image
                    image[pixelRow][pixelCol][2] = (unsigned char)(255); ///red
                    image[pixelRow][pixelCol][1] = (unsigned char)(255); ///green
                    image[pixelRow][pixelCol][0] = (unsigned char)(255); ///blue
                }
            }
        }
        generateBitmapImage((unsigned char *)image, height, width, pitch, imageFileName.c_str());
        writeClassificationFile(classFile, imageFileName, height, width, radius);
        printf("Image %s generated!\n", imageFileName.c_str());
    }
    fclose(classFile);
    printf("Time taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);
    return 0;
}

void generateBitmapImage(unsigned char *image, int height, int width, int pitch,
                         const char* imageFileName)
{
    unsigned char padding[3] = {0, 0, 0};
    int paddingSize = (4 - (pitch) % 4) % 4;

    unsigned char* fileHeader = createBitmapFileHeader(height, width, pitch, paddingSize);
    unsigned char* infoHeader = createBitmapInfoHeader(height, width);

    FILE* imageFile = fopen(imageFileName, "wb");

    fwrite(fileHeader, 1, fileHeaderSize, imageFile);
    fwrite(infoHeader, 1, infoHeaderSize, imageFile);

    int i;

    for(i=0; i<height; i++)
    {
        fwrite(image+(i*pitch), bytesPerPixel, width, imageFile);
        fwrite(padding, 1, paddingSize, imageFile);
    }

    fclose(imageFile);
}

unsigned char* createBitmapFileHeader(int height, int width, int pitch, int paddingSize)
{
    int fileSize = fileHeaderSize + infoHeaderSize + (pitch+paddingSize) * height;

    static unsigned char fileHeader[] =
    {
        0,0, /// signature
        0,0,0,0, /// image file size in bytes
        0,0,0,0, /// reserved
        0,0,0,0, /// start of pixel array
    };

    fileHeader[ 0] = (unsigned char)('B');
    fileHeader[ 1] = (unsigned char)('M');
    fileHeader[ 2] = (unsigned char)(fileSize);
    fileHeader[ 3] = (unsigned char)(fileSize>>8);
    fileHeader[ 4] = (unsigned char)(fileSize>>16);
    fileHeader[ 5] = (unsigned char)(fileSize>>24);
    fileHeader[10] = (unsigned char)(fileHeaderSize + infoHeaderSize);

    return fileHeader;
}

unsigned char* createBitmapInfoHeader(int height, int width)
{
    static unsigned char infoHeader[] =
    {
        0,0,0,0, /// header size
        0,0,0,0, /// image width
        0,0,0,0, /// image height
        0,0, /// number of color planes
        0,0, /// bits per pixel
        0,0,0,0, /// compression
        0,0,0,0, /// image size
        0,0,0,0, /// horizontal resolution
        0,0,0,0, /// vertical resolution
        0,0,0,0, /// colors in color table
        0,0,0,0, /// important color count
    };

    infoHeader[ 0] = (unsigned char)(infoHeaderSize);
    infoHeader[ 4] = (unsigned char)(width);
    infoHeader[ 5] = (unsigned char)(width>>8);
    infoHeader[ 6] = (unsigned char)(width>>16);
    infoHeader[ 7] = (unsigned char)(width>>24);
    infoHeader[ 8] = (unsigned char)(height);
    infoHeader[ 9] = (unsigned char)(height>>8);
    infoHeader[10] = (unsigned char)(height>>16);
    infoHeader[11] = (unsigned char)(height>>24);
    infoHeader[12] = (unsigned char)(1);
    infoHeader[14] = (unsigned char)(bytesPerPixel*8);

    return infoHeader;
}
