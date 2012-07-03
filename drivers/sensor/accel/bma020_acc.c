#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

#include "bma020_acc.h"

/* KR3DM IOCTL */
#define KR3DM_IOC_MAGIC                 'B'
#define KR3DM_SET_RANGE                 _IOWR(KR3DM_IOC_MAGIC,4, unsigned char)
#define KR3DM_SET_MODE                  _IOWR(KR3DM_IOC_MAGIC,6, unsigned char)
#define KR3DM_SET_BANDWIDTH             _IOWR(KR3DM_IOC_MAGIC,8, unsigned char)
#define KR3DM_READ_ACCEL_XYZ            _IOWR(KR3DM_IOC_MAGIC,46,short)
#define KR3DM_IOC_MAXNR                 48
#define KR3DM_MODE_NORMAL      0
#define KR3DM_MODE_SLEEP       2
#define KR3DM_MODE_WAKE_UP     3


// this proc file system's path is "/proc/driver/bma020"
// usage :	(at the path) type "cat bma020" , it will show short information for current accelation
// 			use it for simple working test only

#define BMA020_PROC_FS

#ifdef BMA020_PROC_FS

#include <linux/proc_fs.h>

#define DRIVER_PROC_ENTRY		"driver/bma020"

static int bma020_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *p = page;
	int len;
	bma020acc_t acc;
	bma020_set_mode( BMA020_MODE_NORMAL );
	bma020_read_accel_xyz(&acc);
	p += sprintf(p,"[BMA020]\nX axis: %d\nY axis: %d\nZ axis: %d\n" , acc.x, acc.y, acc.z);
	len = (p - page) - off;
	if (len < 0) {
		len = 0;
	}

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;
	return len;
}
#endif	//BMA020_PROC_FS

/* add by inter.park */
//extern void enable_acc_pins(void);

struct class *acc_class;

/* no use */
//static int bma020_irq_num = NO_IRQ;

/* create bma020 object */
bma020_t bma020;

/* create bma020 registers object */
bma020regs_t bma020regs;

/*************************************************************************/
/*		BMA020 Sysfs	  				         */
/*************************************************************************/
//TEST
static ssize_t bma020_fs_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	int count;
	bma020acc_t accels; 
	bma020_read_accel_xyz( &accels );

	printk("x: %d,y: %d,z: %d\n", accels.x, accels.y, accels.z);
	count = sprintf(buf,"%d,%d,%d\n", accels.x, accels.y, accels.z );

	return count;
}

static ssize_t bma020_fs_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	//buf[size]=0;
	printk("input data --> %s\n", buf);

	return size;
}

static DEVICE_ATTR(acc_file, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, bma020_fs_read, bma020_fs_write);


#if 0
static irqreturn_t bma020_acc_isr( int irq, void *unused, struct pt_regs *regs )
{
	printk( "bma020_acc_isr event occur!!!\n" );
	
	return IRQ_HANDLED;
}
#endif


int bma020_open (struct inode *inode, struct file *filp)
{
	gprintk("\n");
	return 0;
}

ssize_t bma020_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t bma020_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

int bma020_release (struct inode *inode, struct file *filp)
{
	gprintk("\n");
	
	return 0;
}

int bma020_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,  unsigned long arg)
{
	int err = 0;
	unsigned char data[6];

	gprintk("ioctl cmd: %x arg: %x\n", cmd, arg);
	switch(cmd) {
	    case KR3DM_READ_ACCEL_XYZ:
                err = bma020_read_accel_xyz((bma020acc_t*)data);
                if(copy_to_user((bma020acc_t*)arg,(bma020acc_t*)data,6)!=0) {
                        printk("copy_to error\n");
                        return -EFAULT;
                }
                return err;
            case KR3DM_SET_MODE:
                if(copy_from_user(data,(unsigned char*)arg,1)!=0) {
                    printk("[BMA150] copy_from_user error\n");
                    return -EFAULT;
                }
                switch (data[0]) {
                    case KR3DM_MODE_NORMAL:
                      data[0] = BMA020_MODE_NORMAL;
                      break;
                    case KR3DM_MODE_WAKE_UP:
                      data[0] = BMA020_MODE_WAKE_UP;
                      break;
                    case KR3DM_MODE_SLEEP:
                      data[0] = BMA020_MODE_SLEEP;
                      break;
                    default:
                      break;
                }
                err = bma020_set_mode(*data);
                gprintk("set mode to %d\n", *data);
                return err;
	    default:
	      break;
	}
	return 0;

	switch(cmd)
	{
		case BMA150_READ_ACCEL_XYZ:
			err = bma020_read_accel_xyz((bma020acc_t*)data);
			if(copy_to_user((bma020acc_t*)arg,(bma020acc_t*)data,6)!=0)
			{
#if DEBUG
				printk("copy_to error\n");
#endif
				return -EFAULT;
			}
			return err;

		case BMA150_SET_RANGE:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
#if DEBUG           
				printk("[BMA150] copy_from_user error\n");
#endif
				return -EFAULT;
			}
			err = bma020_set_range(*data);
                        printk("[BMA150] set range to %d\n", *data);
			return err;
		
		case BMA150_SET_MODE:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
#if DEBUG           
				printk("[BMA150] copy_from_user error\n");
#endif
				return -EFAULT;
			}
			err = bma020_set_mode(*data);
                        printk("[BMA150] set mode to %d\n", *data);
			return err;

		case BMA150_SET_BANDWIDTH:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
#if DEBUG
				printk("[BMA150] copy_from_user error\n");
#endif
				return -EFAULT;
			}
			err = bma020_set_bandwidth(*data);
                        printk("[BMA150] set bandwidth to %d\n", *data);
			return err;
		
		default:
			return 0;
	}
}

struct file_operations acc_fops =
{
	.owner   = THIS_MODULE,
	.read    = bma020_read,
	.write   = bma020_write,
	.open    = bma020_open,
	.ioctl   = bma020_ioctl,
	.release = bma020_release,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma020_early_suspend(struct early_suspend *handler)
{
	printk( "%s : Set MODE SLEEP\n", __func__ );
	bma020_set_mode( BMA020_MODE_SLEEP );
}

static void bma020_late_resume(struct early_suspend *handler)
{
	printk( "%s : Set MODE NORMAL\n", __func__ );
	bma020_set_mode( BMA020_MODE_NORMAL );
}
#endif /* CONFIG_HAS_EARLYSUSPEND */ 

void bma020_chip_init(void)
{
	/*assign register memory to bma020 object */
	bma020.image = &bma020regs;

	bma020.bma020_bus_write = i2c_acc_bma020_write;
	bma020.bma020_bus_read  = i2c_acc_bma020_read;

#ifdef CONFIG_HAS_EARLYSUSPEND
	bma020.early_suspend.suspend = bma020_early_suspend;
	bma020.early_suspend.resume = bma020_late_resume;
	register_early_suspend(&bma020.early_suspend);
#endif

	/*call init function to set read write functions, read registers */
	bma020_init( &bma020 );

	/* from this point everything is prepared for sensor communication */


	/* set range to 2G mode, other constants: 
	 * 	   			4G: BMA020_RANGE_4G, 
	 * 	    		8G: BMA020_RANGE_8G */

	// Default for Moment Eclaire is 2G
	bma020_set_range(BMA020_RANGE_8G);

	/* set bandwidth to 25 HZ */
	// Default for Moment Eclaire is 25HZ
	bma020_set_bandwidth(BMA020_BW_100HZ);
        //bma020_set_mode( BMA020_MODE_NORMAL );

	/* for interrupt setting */
//	bma020_set_low_g_threshold( BMA020_HG_THRES_IN_G(0.35, 2) );

//	bma020_set_interrupt_mask( BMA020_INT_LG );

}

int bma020_acc_start(void)
{
	int result;

	struct device *dev_t;
	
	bma020acc_t accels; /* only for test */
	
	result = register_chrdev( BMA150_MAJOR, "kr3dm", &acc_fops);

	if (result < 0) 
	{
		return result;
	}
	
	acc_class = class_create (THIS_MODULE, "BMA-dev");
	
	if (IS_ERR(acc_class)) 
	{
		unregister_chrdev( BMA150_MAJOR, "kr3dm" );
		return PTR_ERR( acc_class );
	}

	dev_t = device_create( acc_class, NULL, MKDEV(BMA150_MAJOR, 0), "%s", "kr3dm");

	if (IS_ERR(dev_t)) 
	{
		return PTR_ERR(dev_t);
	}
	
	result = i2c_acc_bma020_init();

	if(result)
	{
		return result;
	}

	bma020_chip_init();

	bma020_read_accel_xyz( &accels );
	gprintk("x = %d  /  y =  %d  /  z = %d\n", accels.x, accels.y, accels.z );

	
#ifdef BMA020_PROC_FS
	create_proc_read_entry(DRIVER_PROC_ENTRY, 0, 0, bma020_proc_read, NULL);
#endif	//BMA020_PROC_FS

	bma020_set_mode(BMA020_MODE_SLEEP);
	return 0;
}

void bma020_acc_end(void)
{
	unregister_chrdev( BMA150_MAJOR, "kr3dm" );
	
	i2c_acc_bma020_exit();

	device_destroy( acc_class, MKDEV(BMA150_MAJOR, 0) );
	class_destroy( acc_class );
	unregister_early_suspend(&bma020.early_suspend);
}


static int bma020_accelerometer_probe( struct platform_device* pdev )
{
	return bma020_acc_start();
}

static int bma020_accelerometer_suspend( struct platform_device* pdev, pm_message_t state )
{
	return 0;
}


static int bma020_accelerometer_resume( struct platform_device* pdev )
{
	return 0;
}


static struct platform_device *bma020_accelerometer_device;

static struct platform_driver bma020_accelerometer_driver = {
	.probe 	 = bma020_accelerometer_probe,
	.suspend = bma020_accelerometer_suspend,
	.resume  = bma020_accelerometer_resume,
	.driver  = {
		.name = "bma020-accelerometer", 
	}
};


static int __init bma020_acc_init(void)
{
	int result;
	printk("%s \n",__func__); 

	result = platform_driver_register( &bma020_accelerometer_driver );

	if( result ) 
	{
		return result;
	}

	bma020_accelerometer_device  = platform_device_register_simple( "bma020-accelerometer", -1, NULL, 0 );
	
	if( IS_ERR( bma020_accelerometer_device ) )
	{
		return PTR_ERR( bma020_accelerometer_device );
	}

	return 0;
}


static void __exit bma020_acc_exit(void)
{
	gprintk("start\n");
	bma020_acc_end();

//	free_irq(bma020_irq_num, NULL);
//	free_irq( IRQ_GPIO( MFP2GPIO( MFP_ACC_INT ) ), (void*)0 );

	platform_device_unregister( bma020_accelerometer_device );
	platform_driver_unregister( &bma020_accelerometer_driver );
}


module_init( bma020_acc_init );
module_exit( bma020_acc_exit );

MODULE_AUTHOR("inter.park");
MODULE_DESCRIPTION("accelerometer driver for BMA020");
MODULE_LICENSE("GPL");
