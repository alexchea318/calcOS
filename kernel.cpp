__asm("jmp kmain");

#define CURSOR_PORT (0x3D4)
#define IDT_TYPE_INTR (0x0E)
#define GDT_CS (0x8)
#define VIDEO_WIDTH (80)
#define IDT_TYPE_TRAP (0x0F)
#define VIDEO_BUF_PTR (0xb8000)
#define N 33

int color = 0x07;
char text_color[40];
int counter_line = 0, counter_pos = 0;

void real_print(const char* ptr, unsigned int strnum);
void move_cursor(unsigned int strnum, unsigned int counter_pos);
void def_intr_handl();

struct idt_entry
{
    unsigned short base_lo;
    unsigned short segm_sel;
    unsigned char always0;
    unsigned char flags;
    unsigned short base_hi;
} __attribute__((packed));

struct idt_ptr
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct idt_entry g_idt[256];
struct idt_ptr g_idtp;


typedef void (*intr_handler)();
void intr_reg_handler(int num, unsigned short segm_sel, unsigned short flags, intr_handler hndlr)
{
    unsigned int hndlr_addr = (unsigned int)hndlr;
    g_idt[num].base_lo = (unsigned short)(hndlr_addr & 0xFFFF);
    g_idt[num].segm_sel = segm_sel;
    g_idt[num].always0 = 0;
    g_idt[num].flags = flags;
    g_idt[num].base_hi = (unsigned short)(hndlr_addr >> 16);
}

void intr_init()
{
    int i;
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
    for (i = 0; i < idt_count; i++)
        intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR, def_intr_handl);
}

void intr_start()
{
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
    g_idtp.base = (unsigned int)(&g_idt[0]);
    g_idtp.limit = (sizeof(struct idt_entry) * idt_count) - 1;
    asm("lidt %0" : : "m" (g_idtp));
}

static inline void for_shooting_down(unsigned int port, unsigned int data)
{
    asm volatile ("outw %w0, %w1" : : "a" (data), "Nd" (port));
}

static inline unsigned char binary_in(unsigned short port)
{
    unsigned char data;
    asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void binary_out(unsigned short port, unsigned char data)
{
    asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

bool mystrcmp(const char* string, int k)
{
    int i;
    unsigned char* ptr_buf_of_video = (unsigned char*)VIDEO_BUF_PTR;
    ptr_buf_of_video += 160 * counter_line + k;
    for (i = 0; string[i] != 0; i++)
    {
        if (string[i] != ptr_buf_of_video[0])
            return 0;
        ptr_buf_of_video += 2;
    }
    if (ptr_buf_of_video[0] && ptr_buf_of_video[0] != ' ')
        return 0;
    return 1;
}

void print_rez(const char* phrase, int flag)
{
    if (flag)
    {
        counter_line++;
    }
    real_print(phrase, counter_line);
    counter_line++;
    real_print("# ", counter_line);
    counter_pos = 2;
    move_cursor(counter_line, counter_pos);
}

void getstr(int k, char* primer)
{
    unsigned char* ptr_buf_of_video = (unsigned char*)VIDEO_BUF_PTR;
    ptr_buf_of_video += 160 * counter_line + k;
    int i = 0;
    while (ptr_buf_of_video[0] != '\0' && i < N)
    {
        primer[i] = ptr_buf_of_video[0];
        i++;
        ptr_buf_of_video += 2;
    }
    primer[i] = '\0';

    //print_rez(primer, 1);
}

int myitoa(int value, char* sp, int radix)
{
    char tmp[16];// be careful with the length of the buffer
    char* tp = tmp;
    int i;
    unsigned v;

    int sign = (radix == 10 && value < 0);
    if (sign)
        v = -value;
    else
        v = (unsigned)value;

    while (v || tp == tmp)
    {
        i = v % radix;
        v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
        if (i < 10)
            *tp++ = i + '0';
        else
            *tp++ = i + 'a' - 10;
    }

    int len = tp - tmp;

    if (sign)
    {
        *sp++ = '-';
        len++;
    }

    while (tp > tmp)
        *sp++ = *--tp;

    return len;
}

int myAtoi(char* str)
{
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i)
        res = res * 10 + str[i] - '0';

    // return result.
    return res;
}

char global_primer[N];
int primer_pos = 0;
int answer_flag = 1;

void result_check(int a, int b, int c, int type)
{
    int test_a = 0;
    if (a > 0) test_a = 1;
    if (a < 0) test_a = -1;
    int test_b = 0;
    if (b > 0) test_b = 1;
    if (b < 0) test_b = -1;
    int test_c = 1;
    if (c < 0) test_c = -1;

    if (type==1)
    {
        if (test_a + test_b > 0 && c > 0) return;
        if (test_a + test_b < 0 && c < 0) return;
    }
    else
    {
        if (test_a * test_b > 0 && c > 0) return;
        if (test_a * test_b < 0 && c < 0) return;
    }
    print_rez("Error: integer overflow", 1);
    answer_flag = 0;
}

char peek()
{
    return global_primer[primer_pos];
}

char get()
{
    return global_primer[primer_pos++];
}

int expression();

int number()
{
    int result = get() - '0';
    while (peek() >= '0' && peek() <= '9')
    {
        result = 10 * result + get() - '0';
    }
    return result;
}

int factor()
{
    if (peek() >= '0' && peek() <= '9')
        return number();
    else if (peek() == '(')
    {
        get(); // '('
        int result = expression();
        get(); // ')'
        return result;
    }
    else if (peek() == '-')
    {
        get();
        return -factor();
    }
    answer_flag = 0;
    print_rez("Error: expression is incorrect [2]", 1);
    return 0; // error
}

int term()
{
    int result = factor();
    while (peek() == '*' || peek() == '/')
    {
        if (get() == '*')
        {
            int a = factor();
            int b = result;
            result *= a;
            result_check(a ,b, result, 2);
        }
        else
        {
            int a = factor();
            if (a == 0)
            {
                print_rez("Error: division by 0", 1);
                answer_flag = 0;
            }
            else
                result /= a;
        }
    }

    return result;
}

int expression()
{
    int result = term();
    while (peek() == '+' || peek() == '-')
    {
        if (get() == '+')
        {
            int a = term();
            int b = result;
            result += a;
            result_check(a, b, result, 1);
        }
        else
            result -= term();
    }

    return result;
}

int check() {

    int j = primer_pos;
    char znaks[] = "+-*/";
    int count = 0;
    while (global_primer[j] != '\0')
    {
        char c = global_primer[j];

        if (c >= '0' && c <= '9')
        {
            count++;
        }

        if (c == '+' || c == '-' || c == '*' || c == '/')
        {
            char d = global_primer[j + 1];
            if (d == '+' || d == '-' || d == '*' || d == '/' || d == '\0')
            {
                return 0;
            }
        }
        j++;
    }
    if (count == 0)
    {
        return 0;
    }
    return 1;
}

void calc(char* primer)
{
    //print_rez("In calc!", 1);
    int j = 0;
    while (primer[j] != '\0')
    {
        global_primer[j] = primer[j];
        j++;
    }
    global_primer[j] = '\0';

    int minus_flag = 0;
    while (global_primer[primer_pos] == '-') {
        primer_pos++;
        minus_flag = 1;
    }
    if (minus_flag)
    {
        primer_pos--;
    }
    if (check())
    {
        int result = expression();
        if (answer_flag)
        {
            char answer[N];
            int len = myitoa(result, answer, 10);
            answer[len] = '\0';
            print_rez(answer, 1);
        }
    }
    else 
    {
        print_rez("Error: expression is incorrect", 1);
    }

}

void main_logic()
{
    if (mystrcmp("info", 4))
    {
        counter_line++;
        real_print("Calc OS: v.01. Developer: Alexander Chechenev, 4851004/90002, SpbPU, 2021", counter_line);
        counter_line++;
        real_print("Compilers: bootloader: gnu, kernel: gcc", counter_line);
        print_rez(text_color, 1);
        return;
    }
    else if (mystrcmp("expr", 4))
    {

        unsigned int offset = 2 * (4 + 3);
        char primer[N];
        getstr(offset, primer);
        //print_rez(primer, 1);

        answer_flag = 1;
        primer_pos = 0;
        calc(primer);
        
        return;
    }
    else if (mystrcmp("shutdown", 4))
    {
        print_rez("here", 1);
        for_shooting_down(0xB004, 0x2000);
        for_shooting_down(0x604, 0x2000);
    }
    print_rez("Unknown command", 1);
}

void get_keys_to_buf(unsigned char key_kod)
{

    if (key_kod == 28)
    {
        main_logic();
        return;
    }
    if (key_kod == 14)
    {
        if (counter_pos <= 2)
        {
            return;
        }
        else
        {
            unsigned char* ptr_buf_of_video = (unsigned char*)VIDEO_BUF_PTR;
            counter_pos--;
            ptr_buf_of_video += 160 * counter_line + 2 * counter_pos;
            ptr_buf_of_video[0] = 0;
            move_cursor(counter_line, counter_pos);
            return;
        }
    }
    unsigned char* ptr_buf_of_video = (unsigned char*)VIDEO_BUF_PTR;

    char per=']';
    if (key_kod== 2) per = '1';
    else if (key_kod == 3) per = '2';
    else if (key_kod == 4) per = '3';
    else if (key_kod == 5) per = '4';
    else if (key_kod == 6) per = '5';
    else if (key_kod == 7) per = '6';
    else if (key_kod == 8) per = '7';
    else if (key_kod == 9) per = '8';
    else  if (key_kod == 10) per = '9';
    else  if (key_kod == 11) per = '0';
    else  if (key_kod == 12) per = '-';
    else  if (key_kod == 0x35) per = '/';
    else  if (key_kod == 0x39) per = ' ';
    else  if (key_kod == 0x4E) per = '+';
    else  if (key_kod == 0x37) per = '*';
    else  if (key_kod == 0x17) per = 'i';
    else  if (key_kod == 0x31) per = 'n';
    else  if (key_kod == 0x21) per = 'f';
    else  if (key_kod == 0x18) per = 'o';
    else  if (key_kod == 0x12) per = 'e';
    else  if (key_kod == 0x2D) per = 'x';
    else  if (key_kod == 0x19) per = 'p';
    else  if (key_kod == 0x13) per = 'r';
    else  if (key_kod == 0x1F) per = 's';
    else  if (key_kod == 0x23) per = 'h';
    else  if (key_kod == 0x16) per = 'u';
    else  if (key_kod == 0x14) per = 't';
    else  if (key_kod == 0x20) per = 'd';
    else  if (key_kod == 0x11) per = 'w';
    else  if (key_kod == 0x1A) per = '(';
    else  if (key_kod == 0x1B) per = ')';

    
    if (per == ']')
        return;

    ptr_buf_of_video += 160 * counter_line + 2 * counter_pos;

    ptr_buf_of_video[0] = per;
    ptr_buf_of_video[1] = color;
    counter_pos++;
    move_cursor(counter_line, counter_pos);
}

void scan_keys()
{
    if (binary_in(0x64) & 0x01)
    {
        unsigned char key_kod, state;
        key_kod = binary_in(0x60);
        if (key_kod < 128)
            get_keys_to_buf(key_kod);
    }
}

void def_intr_handl()
{
    asm("pusha");
    scan_keys();
    binary_out(0x20, 0x20);
    asm("popa; leave; iret");
}

void screan_cleaner()
{
    unsigned char* ptr_buf_of_video = (unsigned char*)VIDEO_BUF_PTR;
    int i;
    for (i = 0; i < 2000; i++)
    {
        ptr_buf_of_video[0] = 0;
        ptr_buf_of_video += 2;
    }
    counter_line = 0;
    real_print("# ", counter_line);
    move_cursor(counter_line, 2);
}

void move_cursor(unsigned int strnum, unsigned int counter_pos)
{
    if (strnum >= 25)
    {
        screan_cleaner();
        return;
    }
    unsigned short new_counter_pos = (strnum * VIDEO_WIDTH) + counter_pos;
    binary_out(CURSOR_PORT, 0x0F);
    binary_out(CURSOR_PORT + 1, (unsigned char)(new_counter_pos & 0xFF));
    binary_out(CURSOR_PORT, 0x0E);
    binary_out(CURSOR_PORT + 1, (unsigned char)((new_counter_pos >> 8) & 0xFF));
}

void real_print(const char* ptr, unsigned int strnum)
{
    unsigned char* ptr_buf_of_video = (unsigned char*)VIDEO_BUF_PTR;
    ptr_buf_of_video += 160* strnum;
    while (*ptr)
    {
        ptr_buf_of_video[0] = (unsigned char)*ptr;
        ptr_buf_of_video[1] = color;
        ptr_buf_of_video += 2;
        ptr++;
    }
}

void mystrcpy(const char* new_color)
{
    char str[] = "Selected color: ";
    int i = 0;
    while (str[i] != '\0')
    {
        text_color[i] = str[i];
        i++;
    }
    int j = i;
    i = 0;
    //unsigned int len = mystrlen("Selected color: ")-1;
    while (new_color[i] != '\0')
    {
        text_color[i + j] = new_color[i];
        i++;
    }
    text_color[i + j] = '\0';
}

void get_color()
{
    unsigned char* ptr_buf_of_video = (unsigned char*)VIDEO_BUF_PTR;

    char str_choise[2];
    ptr_buf_of_video += 20;
    while (ptr_buf_of_video[0] != ':')
    {
        ptr_buf_of_video += 2;
    }
    ptr_buf_of_video += 4;
    str_choise[0] = ptr_buf_of_video[0];
    int choise = myAtoi(str_choise);

    if (choise == 1)
    {
        color = 0x02;
        mystrcpy("green");
    }
    else if (choise == 2)
    {
        color = 0x01;
        mystrcpy("blue");
    }
    else if (choise == 3)
    {
        color = 0x04;
        mystrcpy("red");
    }
    else if (choise == 4)
    {
        color = 0x0E;
        mystrcpy("yellow");
    }
    else if (choise == 5)
    {
        color = 0x07;
        mystrcpy("gray");
    }
    else
    {
        color = 0x0F;
        mystrcpy("white");
    }
}


extern "C" int kmain()
{
    get_color();
    screan_cleaner();
    print_rez("This is CalcOS. Welcome!", 0);

    asm("cli"); //disable intr
    intr_init();
    intr_start();

    asm("sti"); //enable inter

    while (1)
    {
        asm("hlt");
    }
    return 0;
}
