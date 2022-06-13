#include "driver.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
|*                            ATTRIBUTES                             *|
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct RGB *Driver::DATA_RECEIVED = new struct RGB[ARRAY_LENGTH + 1];
int Driver::dataLength = ARRAY_LENGTH;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
|*                          PUBLIC METHODS                           *|
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct RGB *Driver::getAllData()
{
    // Update data
    Driver::updateData();

    struct RGB *data = new struct RGB[ARRAY_LENGTH];

    for (int k = 0; k < ARRAY_LENGTH; k++)
    {
        data[k] = Driver::DATA_RECEIVED[k];
    }

    return data;
}

struct RGB **Driver::getOrderedData()
{
    struct RGB *allData = Driver::getAllData();
    struct RGB dataInfo = DATA_RECEIVED[ARRAY_LENGTH];

    int lastDataPosition = dataInfo.ir;

    cout << allData[lastDataPosition].r << endl;

    struct RGB **orderedArray = (struct RGB **)malloc(ARRAY_LENGTH * sizeof(*orderedArray));

    for (int k = 0; k < ARRAY_LENGTH; k++)
    {
        int pos = (lastDataPosition + 1 + k) % ARRAY_LENGTH;

        struct RGB currentData = allData[pos];

        orderedArray[k] = (struct RGB *)malloc(sizeof(**orderedArray));
        orderedArray[k]->r = currentData.r;
        orderedArray[k]->g = currentData.g;
        orderedArray[k]->b = currentData.b;
        orderedArray[k]->ir = currentData.ir;
    }

    return orderedArray;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
|*                          PRIVATE METHODS                          *|
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void Driver::updateData()
{
    int arrayLength = (ARRAY_LENGTH + 1) * sizeof(struct RGB);

    int fileDevice = open("/dev/drvSenseHat", O_RDONLY);

    if (fileDevice < 0)
    {
        qDebug() << "Impossible d'ouvrir le driver";
        exit(-1);
    }

    int returnValue = read(fileDevice, DATA_RECEIVED, arrayLength);

    if (returnValue < 0)
    {
        qDebug() << "Impossible de lire le driver";
        exit(-1);
    }
}
