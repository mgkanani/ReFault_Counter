#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/timer.h>// for timer api
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mahendra");
MODULE_DESCRIPTION("Project :- Refault counting for page cache");

/*
int pid=0;
module_param(pid,int,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
*/

#define MAX_OFFSET_ENTRY	10000
#define MAX_INODE_ENTRY		100

unsigned long s_time=30;//30 seconds
static struct timer_list t_timer;
spinlock_t list_lock;



static int dbg = 0;

extern void (*ptr_page_cache_remove)(unsigned long, unsigned long);
static void (*orig_ptr_page_cache_remove)(unsigned long, unsigned long);
extern void (*ptr_refault)(unsigned long, unsigned long);
static void (*orig_ptr_refault)(unsigned long, unsigned long);
extern void (*ptr_check_refault)(unsigned long, unsigned long);
static void (*orig_ptr_check_refault)(unsigned long, unsigned long);

/*
static unsigned long long refaults=0;
*/
static unsigned long	total_inodes=0;

struct file_offset{
	unsigned short flag;
	unsigned int refaults_cnt;
	unsigned long offset;
	struct list_head list;	
};

struct pgcache_id {
	unsigned short flag;
	unsigned long offset_cnt;//total number of nodes for this inode/file.
	unsigned long inode;
	struct file_offset sublist;
	struct list_head list;
};


struct pgcache_id mylist;
void find_insert_inode_obj(struct pgcache_id *head, unsigned long inode, unsigned long offset);
int check_inode_obj(struct pgcache_id *head, unsigned long inode, unsigned long offset);
void add_inode_obj(struct pgcache_id *head, unsigned long inode, unsigned long offset);

// 0 :- not active any offset entry otherwise 1.
int remove_entries_per_inode(struct pgcache_id *inode, struct file_offset *head, unsigned long *offset_cnt){
        struct file_offset *tmp;
        struct list_head *pos,*q;
	int ret=0;
        int i=0;
//        printk("In del_sublist()\n");
        list_for_each_safe(pos, q, &head->list){
		//if(inode->flag)
		//	return;
                tmp= list_entry(pos, struct file_offset, list);
		if(tmp->flag){
			tmp->flag=0;
			ret=1;
			continue;
		}
                //printk("inode: %lu\toffset: %lu\tcount: %u\n", tmp->inode, tmp->offset, tmp->cnt);
                list_del(pos);
                kfree(tmp);
                i++;
		(*offset_cnt)--;
        }
	return ret;
}

void remove_entries(struct pgcache_id *head){
	struct pgcache_id *tmp;
        struct list_head *pos,*q;
	int ret=0;
        list_for_each_safe(pos, q, &(head->list)){
                tmp = list_entry(pos, struct pgcache_id, list);
                ret = remove_entries_per_inode(tmp, &(tmp->sublist),&tmp->offset_cnt);
                if( !tmp->flag && ret==0){
                        //remove_entries_per_inode(tmp, &(tmp->sublist));
                	//if( !tmp->flag ){//still flag is not set so remove the inode entry itself.
				list_del(pos);
				kfree(tmp);
				total_inodes--;
			//}
                }else{
			tmp->flag=0;
		}
        }
}

static void reset_flags(unsigned long temp){
	int ret=0;
	remove_entries(&mylist);
	ret = mod_timer( &t_timer, round_jiffies(jiffies + s_time * HZ));
	if(ret)
		printk("Error in mod_timer\n");
	printk("In reset_flags, total_inodes:- %lu\n",total_inodes);

}


//This will be called when a cached page evicted.

void insert_refault_data(unsigned long inode, unsigned long offset){
        if(inode !=0 ){
                find_insert_inode_obj(&mylist, inode, offset);
        }
}

//This will be called when a shadow entry found instead of page i.e. refault occured.
void handle_refaults(unsigned long inode, unsigned long offset){
	int ret=-1;
        if(inode!=0){
		//printk("shadow entry for inode: %lu\n", inode);
		ret = check_inode_obj(&mylist, inode, offset);
		if(ret==0)
                	add_inode_obj(&mylist, inode, offset);
        }

/*
	if(inode==0)
		printk("really!!\n");
	struct pgcache_id tmp;
	refaults++;
	//tmp->cnt = 0;
	tmp.inode = inode;
	tmp.offset = offset;
	find_ele(&tmp,&mylist);
*/
}

//This will be called whenever there is page cache fault.
void check_for_refault(unsigned long inode, unsigned long offset){
	if(inode!=0){
		check_inode_obj(&mylist, inode, offset);
	}
}

void add_offset_obj(struct file_offset *head, unsigned long offset){
	struct file_offset *tmp;
	if(head!=NULL){
		tmp= (struct file_offset *)kmalloc(sizeof(struct file_offset),GFP_KERNEL);
		//initialization
		tmp->offset = offset;
		tmp->refaults_cnt = 0;
		tmp->flag = 1;
		list_add_tail(&(tmp->list),&(head->list));
	}
}

void add_inode_obj(struct pgcache_id *head, unsigned long inode, unsigned long offset){
	struct pgcache_id *tmp;
	if(head!=NULL){
		tmp= (struct pgcache_id *)kmalloc(sizeof(struct pgcache_id),GFP_KERNEL);
		//initialization
		tmp->inode = inode;
		tmp->flag  = 1;
		tmp->offset_cnt = 1;
		INIT_LIST_HEAD(&(tmp->sublist.list));
		add_offset_obj(&tmp->sublist, offset);
		list_add_tail(&(tmp->list),&(head->list));
		total_inodes++;
	}
}

/*
	returns 0 on found otherwise 1.
*/
int find_insert_offset_obj(struct file_offset *head, unsigned long offset){
        struct file_offset *tmp;
        list_for_each_entry(tmp, &(head->list), list){
                if( offset == tmp->offset){
			tmp->flag = 1;//set the flag.
	/*
			spin_lock(&list_lock);
                        tmp->refaults_cnt += 1;
			spin_unlock(&list_lock);
	*/
                        return 0;
                }
        }
	return 1;
}


void find_insert_inode_obj(struct pgcache_id *head, unsigned long inode, unsigned long offset){
	struct pgcache_id *tmp;
	int ret=-1;
	list_for_each_entry(tmp, &(head->list), list){
		if( inode == tmp->inode){
			ret = find_insert_offset_obj(&(tmp->sublist),offset);
			if(ret==1){
				tmp->offset_cnt += 1;
				add_offset_obj(&(tmp->sublist), offset);
			}
			tmp->flag = 1;//set the flag.
			return;
		}
	}
	add_inode_obj(head, inode, offset);
}

/*
only checks for refaults and increment count if found , no insert if not found.
return 1 if found otherwise 0.
*/
int check_offset_obj(struct file_offset *head, unsigned long offset){
        struct file_offset *tmp;
        list_for_each_entry(tmp, &(head->list), list){
                if( offset == tmp->offset){
			spin_lock(&list_lock);
			tmp->flag = 1;//set the flag.
                        tmp->refaults_cnt += 1;
			spin_unlock(&list_lock);
                        //tmp->refaults_cnt += 1;
                        return 1;
                }
        }
        return 0;
}


int  check_inode_obj(struct pgcache_id *head, unsigned long inode, unsigned long offset){
	int ret=-1;
        struct pgcache_id *tmp;
        list_for_each_entry(tmp, &(head->list), list){
                if( inode == tmp->inode){
			tmp->flag = 1;//set the flag.
                        ret = check_offset_obj(&(tmp->sublist),offset);
                        return ret;
                }
        }
	return 0;
}




void del_sublist(struct file_offset *head){
        struct file_offset *tmp;
        struct list_head *pos,*q;
        int i=0;
//        printk("In del_sublist()\n");
        list_for_each_safe(pos, q, &head->list){
                tmp= list_entry(pos, struct file_offset, list);
                //printk("inode: %lu\toffset: %lu\tcount: %u\n", tmp->inode, tmp->offset, tmp->cnt);
                list_del(pos);
                kfree(tmp);
                i++;
        }
//        printk("total deleted=%i\n",i);
}

void del_ele(void){
	struct pgcache_id *tmp;
	struct list_head *pos,*q;
	int i=0;
//	printk("In del_ele()\n");
	list_for_each_safe(pos, q, &mylist.list){
		tmp= list_entry(pos, struct pgcache_id, list);
//		printk("inode: %lu\ttotal_offsets: %lu\n",tmp->inode, tmp->offset_cnt);
		//printk("inode: %lu\toffset: %lu\tcount: %u\n", tmp->inode, tmp->offset, tmp->cnt);
		del_sublist(&tmp->sublist);
		list_del(pos);
		kfree(tmp);
		i++;
		total_inodes--;
	}
//	printk("total deleted=%i\n",i);
}


void print_list_per_inode(struct file_offset *head){
	struct file_offset *tmp;
	list_for_each_entry(tmp, &(head->list), list){
		//if(tmp->refaults_cnt!=0)
		{
			printk("offset: %lu\trefaults: %u\n", tmp->offset, tmp->refaults_cnt);
		}
	}
}

void print_list(struct pgcache_id *head){
	struct pgcache_id *tmp;
	list_for_each_entry(tmp, &(head->list), list){
		printk("inode: %lu\ttotal_offsets: %lu\n",tmp->inode, tmp->offset_cnt);
		print_list_per_inode(&tmp->sublist);
		//printk("inode: %lu\toffset: %lu\tcount: %u\n", tmp->inode, tmp->offset, tmp->cnt);
	}
}

void test(void){
	unsigned long i,j;
	for(i=1;i<3;i++){
		for(j=0;j<10;j++){
			check_for_refault(i,j);
		}
	}
}

void test1(void){
	unsigned long i,j;
	for(i=1;i<3;i++){
		for(j=0;j<10;j++){
			insert_refault_data(i,j);
		}
	}
}

void test2(void){
	unsigned long i,j;
	for(i=1;i<3;i++){
		for(j=0;j<10;j++){
			handle_refaults(i,j);
		}
	}
}

void init_lists(void){
	INIT_LIST_HEAD(&mylist.list);

	init_timer(&t_timer);
        t_timer.function	= reset_flags ;
        t_timer.data = 1;
        //t_timer.expires = jiffies + HZ;
        t_timer.expires = round_jiffies(jiffies + s_time * HZ);
        //t_timer.expires = jiffies + msecs_to_jiffies(s_time);//in milisecs
        add_timer( &t_timer);

}

int init_module(void)
{
	init_lists();
	if(!dbg){

		orig_ptr_page_cache_remove	= ptr_page_cache_remove;
		orig_ptr_refault		= ptr_refault;
		orig_ptr_check_refault		= ptr_check_refault;
		ptr_page_cache_remove		= &insert_refault_data;
		ptr_refault			= &handle_refaults;
		ptr_check_refault		= &check_for_refault;
	}else{
		test();
		print_list(&mylist);
		printk("insertion \n");
		test1();
		print_list(&mylist);
		printk("refaults occured \n");
		test2();
		print_list(&mylist);
		printk("Again refaults occured \n");
		test();
		print_list(&mylist);
	}
	printk("Module inserted offset=%u inode=%u\n",sizeof(struct file_offset),sizeof(struct pgcache_id));
	printk("Output from mylist\n");
/*
*/
	return 0;
}

void cleanup_module(void)
{
        int ret;
	if(!dbg){
		ptr_refault 		= orig_ptr_refault;
		ptr_check_refault	= orig_ptr_check_refault;
		ptr_page_cache_remove	= orig_ptr_page_cache_remove;
	}
        ret = del_timer_sync( &t_timer );
        if(ret)
                printk("The timer is still in use...\n");

	print_list(&mylist);
	del_ele();
	//print_list(&mylist);
	printk(KERN_INFO "Module Unloaded.\n");
}

