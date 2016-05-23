#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/fb.h>
#include <linux/slab.h>

#define CONFIG_DISPLAY_FB 0
//#define CONFIG_BOOT_PROCESS
//#define CONFIG_BOOT_PROCESS_BLOCK
#define CONFIG_BOOT_PROCESS_BAR

#define CONFIG_PROCESS_DURATION 10

static struct delayed_work boot_process_work;
static u32 palette[5] = {200, 203, 206, 209, 212};

static void draw_box(int x, int y, int width, int height, int color_index)
{
    struct fb_info * fb;
    struct fb_image process;
    char * data;
    if (color_index >= sizeof(palette) / sizeof(u32)) {
        printk("color_index out of range!\n");
        return ;
    }
    data = kmalloc(width * height, GFP_KERNEL);
    memset(data, color_index, width * height);
    process.dx = x;
    process.dy = y;
    process.width = width;
    process.height= height;
    process.data = data;
    process.depth = 8;
    fb = registered_fb[CONFIG_DISPLAY_FB];
    fb->pseudo_palette = palette;
    fb->fbops->fb_imageblit(fb, &process);
    kfree(data);
}

#if (defined CONFIG_BOOT_PROCESS_BAR) && (defined CONFIG_BOOT_PROCESS)
#define PROCESS_BLOCK_NUM 20
static void process_worker(struct work_struct * work)
{
    static unsigned long timestamp = 0;
    static int process_width;
    static int process_height;
    static int block_num_adjust;
    static int block_width;
    static int process_x;
    static int process_y;
    static int step;
    if ( registered_fb[CONFIG_DISPLAY_FB] == NULL ) {
        printk("Boot process: fb dev not inited, boot process not start!\n"); 
        return ;
    }
    /*  fisrt call process_worker, set timestamp as jiffies */
    if (timestamp == 0) {
        int screen_width; 
        int screen_height;
        timestamp = jiffies;
        step = 0; 
        screen_width = registered_fb[CONFIG_DISPLAY_FB]->var.xres;
        screen_height = registered_fb[CONFIG_DISPLAY_FB]->var.yres;
        /*  draw process bar background                     */
        process_width   = screen_width * 4 / 5;
        process_height  = screen_height / 10;
        process_x       = (screen_width - process_width) / 2;
        process_y       =  screen_height - process_height * 2;
        block_width     = process_width / PROCESS_BLOCK_NUM;
        block_num_adjust = process_width / block_width;
        draw_box(process_x, process_y, process_width, process_height, 0);
    } else {
        int i;
        int current_x;
        step++;
        for (i = 0 ; i < 4; i++) {
            current_x = process_x + block_width * ( step -i );
            if (current_x >= (process_x + process_width - 2 * block_width) || current_x < process_x) {
                continue; 
            }
            draw_box(current_x, process_y, block_width, process_height, i + 1);
        }
    }
    if (jiffies - timestamp > CONFIG_PROCESS_DURATION * HZ) {
        draw_box(process_x, process_y, process_width, process_height, 4);
        //printk("Boot process: boot process end!\n");			//modefy by fck zlgmcu
        return ; 
    }
    schedule_delayed_work(&boot_process_work, CONFIG_PROCESS_DURATION * HZ / (block_num_adjust) );
}
#endif

#if (defined CONFIG_BOOT_PROCESS_BLOCK) && (defined CONFIG_BOOT_PROCESS)
static void process_worker(struct work_struct * work)
{
    static unsigned long timestamp = 0;
    static int start_x;
    static int start_y;
    static int count = 0;
    int screen_width; 
    int screen_height;
    int width;
    int height;
    int span;
    int tmp;
    int i;

    if ( registered_fb[CONFIG_DISPLAY_FB] == NULL ) {
        printk("Boot process: fb dev not inited, boot process not start!\n"); 
        return ;
    }
    screen_width = registered_fb[CONFIG_DISPLAY_FB]->var.xres;
    screen_height = registered_fb[CONFIG_DISPLAY_FB]->var.yres;
    /*  draw process bar background                     */
    tmp = screen_width * 4 / 5;
    height  = screen_height / 10;
    width = height;
    start_x = (screen_width - tmp) / 2;
    start_y =  screen_height - height * 2;
    span = (tmp - width) / 4;

    /*  fisrt call process_worker, set timestamp as jiffies */
    timestamp = (timestamp == 0) ? jiffies : timestamp;
    if (jiffies - timestamp > CONFIG_PROCESS_DURATION * HZ) {
        for (i = 0; i < 5; i++) {
            draw_box(start_x + i * span, start_y, width, height, 4); 
        }
        printk("Boot process: boot process end!\n");
        return ; 
    }
    tmp = ++count;
    count = tmp % 5;
    for (i = 0; i < 5; i++) {
        if (count == i) {
            draw_box(start_x + i * span, start_y, width, height, 4); 
        } else {
            draw_box(start_x + i * span, start_y, width, height, 0); 
        }
    }
    schedule_delayed_work(&boot_process_work, HZ);
}
#endif

int boot_process_init(void)
{
#if defined CONFIG_BOOT_PROCESS
    INIT_DELAYED_WORK(&boot_process_work, process_worker);
    schedule_delayed_work(&boot_process_work, HZ);
    printk("Boot process: init\n");
#endif
    return 0;
}
fs_initcall(boot_process_init);
