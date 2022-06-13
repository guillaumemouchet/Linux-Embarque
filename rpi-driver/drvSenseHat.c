/***************************************************************************/
/**
 *  \file       drvSenseHat.c
 *
 *  \details    Simple I2C driver explanation
 *
 *  \author     Base FSA, Update Vincent Jeannin & Guillaume Mouchet
 *
 * *******************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/kthread.h>

#include <linux/fs.h>                               /* Header for the Linux file system support 	                */
#include <linux/uaccess.h>                          /* Required for the copy to user function		                */
#include <linux/moduleparam.h>                      /* Needed for module parameters 				                */
#include <linux/device.h>                           /* Header to support the kernel Driver Model	                */

#define I2C_BUS_AVAILABLE (1)                       /* I2C Bus available in our Raspberry Pi                        */
#define SLAVE_DEVICE_NAME ("TCS34725")              /* Device and Driver Name                                       */
#define TCS_SLAVE_ADDR (0x29)                       /* TCS34725 Slave Address                                       */

#define DEVICE_NAME "drvSenseHat"                   /* The device will appear at /dev/drvSenseHat using this value  */
#define CLASS_NAME "hearc"                          /* The device class -- this is a character device driver	    */
#define SIZE_OF_ARRAY 100

static struct i2c_adapter *tcs_i2c_adapter = NULL;  /* I2C Adapter Structure                                        */
static struct i2c_client *tcs_i2c_client = NULL;    /* I2C Cient Structure                                          */

static struct RGB
{
    int r;
    int g;
    int b;
    int ir;
};

static int majorNumber;                             /* Device number -- determined automatically			        */
static struct RGB luxArray[SIZE_OF_ARRAY + 1];      /* Memory for the string that is passed from userspace	        */
static int nbrOpens = 0;                            /* Counts the number of times the device is opened		        */
static struct class *devSenseHatClass = NULL;       /* The device-driver class struct pointer				        */
static struct device *devSenseHatDevice = NULL;     /* The device-driver device struct pointer				        */
static struct task_struct *ts1;                     /* Thread for main function */

static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static int mount_point(void);

/*
** This function writes the data into the I2C client
**
**  Arguments:
**      buff -> buffer to be sent
**      len  -> Length of the data
**
*/
static int I2C_Write(unsigned char *buf, unsigned int len)
{
    /*
    ** Sending Start condition, Slave address with R/W bit,
    ** ACK/NACK and Stop condtions will be handled internally.
    */
    int ret = i2c_master_send(tcs_i2c_client, buf, len);

    return ret;
}

/*
** This function reads one byte of the data from the I2C client
**
**  Arguments:
**      out_buff -> buffer wherer the data to be copied
**      len      -> Length of the data to be read
**
*/
static int I2C_Read(unsigned char *out_buf, unsigned int len)
{
    /*
    ** Sending Start condition, Slave address with R/W bit,
    ** ACK/NACK and Stop condtions will be handled internally.
    */
    int ret = i2c_master_recv(tcs_i2c_client, out_buf, len);

    return ret;
}

/*
** This function sends the command/data to the TCS.
**
**  Arguments:
**      data   -> data to be written
**
*/
static void TCS_Write(unsigned char *data)
{
    int ret;

    ret = I2C_Write(data, 2);
}

/*
** This function reads the data to the TCS.
**
**  Arguments:
**      data   -> data to be read
**
*/
static void TCS_Read(unsigned char *data)
{
    int ret;
    // Read 8 bytes of data from register(0x94)
    // cData lsb, cData msb, red lsb, red msb, green lsb, green msb, blue lsb, blue msb
    char reg[1] = {0x94};

    ret = I2C_Write(reg, 1);
    ret = I2C_Read(data, 8);
}

int thread_function(void *pv)
{
    int i = 0;
    char data[8] = {0};
    int cData;
    int red, green, blue;

    while (true)
    {
        pr_info("Thread Function %d\n", i);
        msleep(1000);
        TCS_Read(data);

        // Convert the data
        cData = (data[1] * 256 + data[0]);
        red = (data[3] * 256 + data[2]);
        green = (data[5] * 256 + data[4]);
        blue = (data[7] * 256 + data[6]);

        luxArray[i].r = red;
        luxArray[i].g = green;
        luxArray[i].b = blue;
        luxArray[i].ir = cData;

        luxArray[SIZE_OF_ARRAY].ir = i;

        // les afficher dans pr_info
        pr_info("Indice : %d\n", luxArray[SIZE_OF_ARRAY].ir);
        pr_info("Red color luminance : %d lux \n", red);
        pr_info("Green color luminance : %d lux \n", green);
        pr_info("Blue color luminance : %d lux \n", blue);
        pr_info("IR  luminance : %d lux \n", cData);

        if (++i >= SIZE_OF_ARRAY)
        {
            i = 0;
        }
    }
}

/*
** This function sends the commands that need to used to Initialize the TCS.
**
**  Arguments:
**      none
**
*/
static int TCS_init(void)
{
    char config[2] = {0};
    char data[8] = {0};
    int cData;
    int red, green, blue;

    msleep(10); // delay

    //
    // Commands to initialize the TCS
    //

    // Select enable register(0x80)
    // Power ON, RGBC enable, wait time disable(0x03)
    config[0] = 0x80;
    config[1] = 0x03;
    TCS_Write(config);
    // Select ALS time register(0x81)
    // Atime = 700 ms(0x00)
    config[0] = 0x81;
    config[1] = 0x00;
    TCS_Write(config);
    // Select Wait Time register(0x83)
    // WTIME : 2.4ms(0xFF)
    config[0] = 0x83;
    config[1] = 0xFF;
    TCS_Write(config);
    // Select control register(0x8F)
    // AGAIN = 1x(0x00)
    config[0] = 0x8F;
    config[1] = 0x00;
    TCS_Write(config);
    msleep(10);

    int err;

    ts1 = kthread_run(thread_function, NULL, "thread-1");
    if (IS_ERR(ts1))
    {
        printk(KERN_INFO "ERROR: Cannot create thread ts1\n");
        err = PTR_ERR(ts1);
        ts1 = NULL;
        return err;
    }

    return 0;
}

/** @brief Devices are represented as file structure in the kernel.
 *  The file_operations structure from /linux/fs.h lists the callback functions that you wish to
 *  associated with your file operations using a C99 syntax structure.
 *  char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
    {
        .open = dev_open,
        .read = dev_read,
        .release = dev_release,
};

/** @brief The mounting point function
 *  The static keyword restricts the visibility of the function to within this C file.
 *  The __init macro means that for a built-in driver the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int mount_point(void)
{
    pr_info("devSenseHat: Initializing the devSenseHat driver\n");

    // Try to dynamically allocate a major number for the device
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0)
    {
        pr_alert("devSenseHat: failed to register a major number\n");
        return majorNumber;
    }
    pr_info("devSenseHat: registered (major number %d)\n", majorNumber);

    // Register the device class
    devSenseHatClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(devSenseHatClass))
    { // Check for error and clean up if there is
        unregister_chrdev(majorNumber, DEVICE_NAME);
        pr_alert("Failed to register device class\n");
        return PTR_ERR(devSenseHatClass); // Return an error on a pointer
    }
    pr_info("devSenseHat: device class registered\n");

    // Register the device driver
    devSenseHatDevice = device_create(devSenseHatClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(devSenseHatDevice))
    { // Clean up if there is an error
        class_destroy(devSenseHatClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        pr_alert("Failed to create the device\n");
        return PTR_ERR(devSenseHatDevice);
    }
    pr_info("devSenseHat: device class created\n");
    return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    int error_count = 0;
    // copy_to_user has the format ( * to, *from, size) and returns 0 on success
    error_count = copy_to_user(buffer, luxArray, (SIZE_OF_ARRAY + 1) * sizeof(struct RGB));

    if (error_count == 0)
    { // if true then have success
        pr_info("devSensHat: Sent %d characters to the user\n", SIZE_OF_ARRAY);
        return 0; // clear the position to the start and return 0
    }
    else
    {
        pr_info("devSensHat: Failed to send %d characters to the user\n", error_count);
        return -EFAULT; // Failed -- return a bad address message (i.e. -14)
    }
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the nbrOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep)
{
    nbrOpens++;
    pr_info("devSenseHat: Device has been opened %d time(s)\n", nbrOpens);
    return 0;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep)
{
    pr_info("devSenseHat: Device closed\n");
    return 0;
}

/*
** This function getting called when the slave has been found
** Note : This will be called only once when we load the driver.
*/
static int tcs_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    pr_info("TCS Probed!!!\n");

    mount_point();

    return 0;
}

/*
** This function getting called when the slave has been removed
** Note : This will be called only once when we unload the driver.
*/
static int tcs_remove(struct i2c_client *client)
{
    pr_info("TCS Removed!!!\n");
    return 0;
}

/*
** Structure that has slave device id
*/
static const struct i2c_device_id tcs_id[] = {
    {SLAVE_DEVICE_NAME, 0},
    {}};
MODULE_DEVICE_TABLE(i2c, tcs_id);

/*
** I2C driver Structure that has to be added to linux
*/
static struct i2c_driver tcs_driver = {
    .driver = {
        .name = SLAVE_DEVICE_NAME,
        .owner = THIS_MODULE,
    },
    .probe = tcs_probe,
    .remove = tcs_remove,
    .id_table = tcs_id,
};

/*
** I2C Board Info strucutre
*/
static struct i2c_board_info tcs_i2c_board_info = {
    I2C_BOARD_INFO(SLAVE_DEVICE_NAME, TCS_SLAVE_ADDR)};

/*
** Module Init function
*/
static int __init tcs_driver_init(void)
{
    int ret = -1;
    tcs_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);

    if (tcs_i2c_adapter != NULL)
    {
        tcs_i2c_client = i2c_new_client_device(tcs_i2c_adapter, &tcs_i2c_board_info);
        if (tcs_i2c_client != NULL)
        {
            i2c_add_driver(&tcs_driver);
            ret = 0;
        }

        i2c_put_adapter(tcs_i2c_adapter);
    }
    TCS_init();
    pr_info("Driver Added!!!\n");
    return ret;
}

/*
** Module Exit function
*/
static void __exit tcs_driver_exit(void)
{
    kthread_stop(ts1);

    i2c_unregister_device(tcs_i2c_client);
    i2c_del_driver(&tcs_driver);
    pr_info("Driver Removed!!!\n");
}

module_init(tcs_driver_init);
module_exit(tcs_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FSA <blabla@gmail.com>");
MODULE_DESCRIPTION("Simple I2C driver for TCS34725");
MODULE_VERSION("1.10");