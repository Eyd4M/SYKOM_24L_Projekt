#include <linux/kernel.h>
#include <linux/module.h> 
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
#include <asm/io.h>


//Definicje bazowe
#define BUF_SIZE 100

//Adres bazowy sykt z pliku .dts
#define SYKT_GPIO_BASE_ADDR (0x00100000)

#define SYKT_GPIO_SIZE (0x8000)
#define SYKT_EXIT (0x3333)
#define SYKT_EXIT_CODE (0x7F)

//Defnicje ściśle pod projekt - adresy rejestrów

//Rejestr A - ZAPIS numeru liczby pierwszej do wyznaczenia
#define SYKT_A_OFFSET   (0xD4)
#define SYKT_A_FILENAME ("rejdoladaA")

//Rejestr S - ODCZYT stanu automatu
#define SYKT_S_OFFSET    (0xEC)
#define SYKT_S_FILENAME  ("rejdoladaS")

//Rejestr W - ODCZYT wyznaczonej liczby pierwszej
#define SYKT_W_OFFSET    (0xE4)
#define SYKT_W_FILENAME  ("rejdoladaW")

//Deklaracje wskaźników
void __iomem *baseptr_finisher;  //FINISHER  
void __iomem *baseptr_a; //do zapisu A
void __iomem *baseptr_s; //do oczytu S 
void __iomem *baseptr_w; //do odczytu W 

//Funkcja zapsisu do rejestru A
static ssize_t write_a_function(struct file *file, const char __user *ubuf, size_t count, loff_t *offset) {
	int num,c,i;
	char buf[BUF_SIZE];
	if(*offset>0 || count>BUF_SIZE){return -EPERM;}
	if(copy_from_user(buf, ubuf, count)){return -EFAULT;}
	buf[count]='\0';
	num=sscanf(buf, "%x", &i);
	if(num!=1){return -EFAULT;}
	writel(i, baseptr_a);
	c=strlen(buf);
	*offset+=count;
	return c;
}

//Funkcja odczytu z rejestru S
static ssize_t read_s_function(struct file *file, char __user *ubuf, size_t count, loff_t *offset) {
    int i;
    char buf[BUF_SIZE];
    ssize_t len;
    if (*offset > 0) {return 0;}
    i = readl(baseptr_s);
    len = snprintf(buf, BUF_SIZE, "%x\n", i);
    if (len > count) {
        len = count;
    }
    if (copy_to_user(ubuf, buf, len)) {return -EFAULT;}
    *offset = len;
    return len;
}

//Funkcja odczytu z rejestru W
static ssize_t read_w_function(struct file *file, char __user *ubuf, size_t count, loff_t *offset) {
    int i;
    char buf[BUF_SIZE];
    ssize_t len;
    if (*offset > 0) {return 0;}
    i = readl(baseptr_w);
    len = snprintf(buf, BUF_SIZE, "%x\n", i);
    if (len > count) {
        len = count;
    }
    if (copy_to_user(ubuf, buf, len)) {return -EFAULT;}
    *offset = len;
    return len;
}

//Struktura danych do obsługi rejestru A
static struct file_operations a_write = {
	.owner = THIS_MODULE,
	.write = write_a_function,
};
//Struktura danych do obsługi rejestru S
static struct file_operations s_read = {
	.owner = THIS_MODULE,
    .read = read_s_function,
};
//Struktura danych do obsługi rejestru W
static struct file_operations w_read = {
	.owner = THIS_MODULE,
    .read = read_w_function,
};

struct proc_dir_entry *parent;
static struct proc_dir_entry *ent1;
static struct proc_dir_entry *ent2;
static struct proc_dir_entry *ent3;

//Funkcja inicjalizująca moduł kernela
static int procfs_init(void){
	printk(KERN_ALERT "Inicjowanie modułu...\n");
	parent=proc_mkdir("sykom", NULL);
    //0220 - zapis; 0440 - odczyt
	if(parent!=NULL){
        //Tworzenie wpisów
		ent1 = proc_create(SYKT_A_FILENAME, 0220, parent, &a_write);
		ent2 = proc_create(SYKT_W_FILENAME, 0440, parent, &w_read);
		ent3 = proc_create(SYKT_S_FILENAME, 0440, parent, &s_read);

        //Obsługa błędów tworzenia wpisów
		if((ent1==NULL)||(ent2==NULL)||(ent3==NULL)){
			printk(KERN_ERR "Nie udało się stworzyć plików systemu proc.\n");
			if (ent1){proc_remove(ent1);}
			if (ent2){proc_remove(ent2);}
			if (ent3){proc_remove(ent3);}
			if (parent){proc_remove(parent);}
            return -ENOMEM;
		}
		
        //Tworzenie wskaźnika do kodu wyjścia
		//baseptr_finisher=ioremap(SYKT_GPIO_BASE_ADDR + SYKT_EXIT_CODE, SYKT_GPIO_SIZE);
        baseptr_finisher=ioremap(SYKT_GPIO_BASE_ADDR, SYKT_GPIO_SIZE);
		if(baseptr_finisher==NULL){
            //Obsługa błędów tworzenia wskaźnika
			printk(KERN_ERR "Nie udało się zmapować pamięci wskaźnika finishera.\n");
			if (baseptr_finisher){
				iounmap(baseptr_finisher);}
                return -ENOMEM;
		}
		
        //Tworzenie wskaźników do rejestrów
		baseptr_a=ioremap(SYKT_GPIO_BASE_ADDR + SYKT_A_OFFSET, SYKT_GPIO_SIZE);
		baseptr_w=ioremap(SYKT_GPIO_BASE_ADDR + SYKT_W_OFFSET, SYKT_GPIO_SIZE);
		baseptr_s=ioremap(SYKT_GPIO_BASE_ADDR + SYKT_S_OFFSET, SYKT_GPIO_SIZE);

        //Obsługa błędów tworzenia wskaźników do rejestrów
		if (!baseptr_a || !baseptr_w || !baseptr_s) {
			printk(KERN_ERR "Nie udało się zmapować pamięci wskaźników obsługujących rejestry.\n");
			if (baseptr_a){iounmap(baseptr_a);}
			if (baseptr_w){iounmap(baseptr_w);}
			if (baseptr_s){iounmap(baseptr_s);}
            return -ENOMEM;
		}
	}else{
		printk(KERN_ERR "Nie udało się stworzyć katalogu proc.\n");
		return -ENOMEM;
	}
  printk(KERN_ALERT "Moduł poprawnie zainicjowany!\n");
	return 0;
}

static void procfs_cleanup(void){
	printk(KERN_WARNING "Zatrzymywanie modułu...\n");
    //Usuwanie wpisów(plików)
	proc_remove(ent1);
	proc_remove(ent2);
	proc_remove(ent3);
    //Usuwanie katalogu sykom
	remove_proc_entry("sykom", NULL);
    //Usuwanie wskaźników
    iounmap(baseptr_a);
    iounmap(baseptr_w);
    iounmap(baseptr_s);
	writel(SYKT_EXIT | ((SYKT_EXIT_CODE)<<16), baseptr_finisher);
    iounmap(baseptr_finisher);
}

//Wywołanie głównych funkcji
module_init(procfs_init);
module_exit(procfs_cleanup);

MODULE_INFO(intree, "Y");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Adam Dolowy");
MODULE_DESCRIPTION("Modul kernela stworzony w ramach projektu z przedmiotu SYKOM 24L");