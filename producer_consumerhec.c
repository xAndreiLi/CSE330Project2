#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched/signal.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/timekeeping.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrei Li & Hector Durazo");
MODULE_DESCRIPTION("Producer and Consumer");

// Initialize module parameters
int uid;
int buff_size;
int p;
int c;
module_param(uid, int, 0);
module_param(buff_size, int, 0);
module_param(p, int, 0);
module_param(c, int, 0);

// Define list and buffer
struct task_struct_list
{
	struct task_struct *task;
	struct task_struct_list *next;
};

struct buffer
{
	int size;
	struct task_struct_list *list;
}

// Global Variables
struct buffer *buff;				  // Buffer for processes
struct semaphore mutex;				  // determines whether producer or consumer accesses
struct semaphore empty;				  // keeps track of unoccupied slots
struct semaphore full;				  // keeps track of # of items in buffer
struct task_struct_list tHead = NULL; // List of threads
u64 elapsed_time;					  // Tracks total time of all processes
int consumed = 0;					  // Tracks number of consumed processes

static int producer(void *arg)
{
	struct task_struct *process; // Pointer to current process
	int produced = 0;			 // Tracks number of produced processes

	// Iterates through all processes with process pointer
	for_each_process(process)
	{
		// Check if process belongs to desired UID
		if (process->cred->uid.val == uid)
		{
			// Exits if signal has been passed already and decrements
			if (down_interruptible(&empty) || down_interruptible(&mutex))
				break;
			// Increment number of produced items
			produced++;
			// Critical Section
			struct task_struct_list *list;
			// If buffer is empty, set the head of the list to a new process
			if (buff->list == NULL)
			{
				list = kmalloc(sizeof(struct task_struct_list), GFP_KERNEL);
				buff->list = list;
			}
			else
			{
				// Iterate to the end of the buffer
				struct task_struct_list *temp = buff->list;
				while (temp->next != NULL)
				{
					temp = temp->next;
				}
				// Make new process at the end of the buffer
				list = kmalloc(sizeof(struct task_struct_list), GFP_KERNEL);
				temp->next = list;
			}
			list->task = process;
			buff->size++;
			printk(KERN_INFO "[%s] Produced Item#-%d at buffer index:%d for PID:%d", current->comm, produced, buff->size, list->task->pid);

			// Signal and increment mutex and full
			up(&mutex);
			up(&full);
		}
	}
	return 0;
}

static int consumer(void *arg)
{
	while (!kthread_should_stop())
	{
		// Exits if signal has been passes already and decrements
		if (down_interruptible(&full) || down_interruptible(&mutex))
			break;
		if (buff != NULL && buff->list != NULL)
		{
			// Get time elapsed and add to total
			u64 time = ktime_get_ns() - buff->list->task->start_time;
			elapsed_time += time;
			// Remove process from buffer
			buff->list = buff->list->next;
			consumed++;
			printk(KERN_INFO "[%s] Consumed Item#-%d on buffer index:%d PID:%d Elapsed Time- %d:%d:%d\n", current->comm, consumed, buff->size, temp->task->pid, hours, minutes, remaining_seconds);
			buff->size--;
			// Signal and increment mutex and full
			up(&mutex);
			up(&empty);
		}
	}
	return 0;
}

static int __init init_func(void)
{
	// Producer and Consumer thread pointers
	struct task_struct *pThread, *cThread;
	// Initialize semaphores
	sema_init(&mutex, 1);		  // Initialize mutex to 1 (Producer)
	sema_init(&empty, buff_size); // Initialize empty to buffer size
	sema_init(&full, 0);		  // Initialize full to 0
	// Allocate buffer memory
	buff = kmalloc(sizeof(struct buffer), GFP_KERNEL);
	// Start Threads
	for (int i = 0; i < p; i++)
	{

		pThread = kthread_run(producer, NULL, "Producer-%d", i);
	}
	for (int i = 0; i < c; i++)
	{
		cThread = kmalloc(sizeof(struct task_struct_list), GFP_KERNEL);
		cThread->task = kthread_run(consumer, NULL, "Consumer-%d", i)

		cThread->next = tHead;
		tHead = cThread;
	}

	struct tastk_struct_list *cThrd = tHead;
	while (cThrd != NULL)
	{
		cThrd = cThrd->next;
	}
	
	return 0;
}

static void __exit exit_func(void)
{
	struct task_struct_list *temp = buff->list;
	while (temp != NULL)
	{
		struct task_struct_list *tb = temp;
		temp = temp->next;
		kfree(tb);
	}
	kfree(buff);

	struct task_struct_list *cThread = tHead;
	while (cThread != NULL)
	{
		for(int i = c; i >= 0; i--)
		{
			up(&full);
			up(&mutex);
			up(&full);
			up(&mutex);
		}
		up(&full);
		up(&mutex);
		kthread_stop(cThread->taxk);
		up(&full);
		up(&mutex);
		cThread = cThead->next;
	}
	
	int seconds = elapsed_time / 1000000000;
	int minutes = seconds / 60;
	int hourse = minutes / 60;
	seconds = seconds % 60;
	minutes = minutes % 60;

	printk(KERN_INFO "The total elapsed time of all processes for UID %d is %d:%d:%d\n", uid, hours, minutes, seconds);
	printk(KERN_INFO "Exiting producer_consumer module\n");

}

module_init(init_func);
module_exit(exit_func);