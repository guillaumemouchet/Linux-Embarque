#include <iostream>

/* Includes for driver */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define ARRAY_LENGTH 100

struct RGB
{
    int r;
    int g;
    int b;
    int ir;
};

static struct RGB dataReceived[ARRAY_LENGTH + 1];

using namespace std;

int main()
{
    int arrayLength = (ARRAY_LENGTH + 1) * sizeof(struct RGB);

    int fileDevice = open("/dev/drvSenseHat", O_RDONLY);

    if (fileDevice < 0)
    {
        cout << "Impossible d'ouvrir le driver" << endl;
        return errno;
    }

    cout << "Lecture des données en cours..." << endl;

    int returnValue = read(fileDevice, dataReceived, arrayLength);

    if (returnValue < 0)
    {
        cout << "Lecture impossible..." << endl;
        return errno;
    }

    struct RGB dataInfo = dataReceived[ARRAY_LENGTH];
    int currentPosition = dataInfo.ir;

    cout << "RED : " << dataReceived[currentPosition].r << endl
         << "GREEN : " << dataReceived[currentPosition].g << endl
         << "BLUE : " << dataReceived[currentPosition].b << endl;

    return 0;
}