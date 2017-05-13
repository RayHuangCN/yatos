/*************************************************
 *   Author: Ray Huang
 *   Date  : 2017/3/31
 *   Email : rayhuang@126.com
 *   Desc  : tty control
 ************************************************/

/********* header files *************************/
#include <yatos/tty.h>
#include <arch/vga.h>
#include <yatos/task.h>
#include <yatos/schedule.h>
#include <yatos/tools.h>
#include <yatos/irq.h>
#include <yatos/errno.h>
#include <yatos/signal.h>
static uint32 keymap[NR_SCAN_CODES * MAP_COLS] = {

/* scan-code			!Shift		Shift		E0 XX	*/
/* ==================================================================== */
/* 0x00 - none		*/	0,		0,		0,
/* 0x01 - ESC		*/	ESC,		ESC,		0,
/* 0x02 - '1'		*/	'1',		'!',		0,
/* 0x03 - '2'		*/	'2',		'@',		0,
/* 0x04 - '3'		*/	'3',		'#',		0,
/* 0x05 - '4'		*/	'4',		'$',		0,
/* 0x06 - '5'		*/	'5',		'%',		0,
/* 0x07 - '6'		*/	'6',		'^',		0,
/* 0x08 - '7'		*/	'7',		'&',		0,
/* 0x09 - '8'		*/	'8',		'*',		0,
/* 0x0A - '9'		*/	'9',		'(',		0,
/* 0x0B - '0'		*/	'0',		')',		0,
/* 0x0C - '-'		*/	'-',		'_',		0,
/* 0x0D - '='		*/	'=',		'+',		0,
/* 0x0E - BS		*/	'\b',	    '\b',	    0,
/* 0x0F - TAB		*/	'\t',		'\t',		0,
/* 0x10 - 'q'		*/	'q',		'Q',		0,
/* 0x11 - 'w'		*/	'w',		'W',		0,
/* 0x12 - 'e'		*/	'e',		'E',		0,
/* 0x13 - 'r'		*/	'r',		'R',		0,
/* 0x14 - 't'		*/	't',		'T',		0,
/* 0x15 - 'y'		*/	'y',		'Y',		0,
/* 0x16 - 'u'		*/	'u',		'U',		0,
/* 0x17 - 'i'		*/	'i',		'I',		0,
/* 0x18 - 'o'		*/	'o',		'O',		0,
/* 0x19 - 'p'		*/	'p',		'P',		0,
/* 0x1A - '['		*/	'[',		'{',		0,
/* 0x1B - ']'		*/	']',		'}',		0,
/* 0x1C - CR/LF		*/	'\n',		'\n',		PAD_ENTER,
/* 0x1D - l. Ctrl	*/	CTRL_L,		CTRL_L,		CTRL_R,
/* 0x1E - 'a'		*/	'a',		'A',		0,
/* 0x1F - 's'		*/	's',		'S',		0,
/* 0x20 - 'd'		*/	'd',		'D',		0,
/* 0x21 - 'f'		*/	'f',		'F',		0,
/* 0x22 - 'g'		*/	'g',		'G',		0,
/* 0x23 - 'h'		*/	'h',		'H',		0,
/* 0x24 - 'j'		*/	'j',		'J',		0,
/* 0x25 - 'k'		*/	'k',		'K',		0,
/* 0x26 - 'l'		*/	'l',		'L',		0,
/* 0x27 - ';'		*/	';',		':',		0,
/* 0x28 - '\''		*/	'\'',		'"',		0,
/* 0x29 - '`'		*/	'`',		'~',		0,
/* 0x2A - l. SHIFT	*/	SHIFT_L,	SHIFT_L,	0,
/* 0x2B - '\'		*/	'\\',		'|',		0,
/* 0x2C - 'z'		*/	'z',		'Z',		0,
/* 0x2D - 'x'		*/	'x',		'X',		0,
/* 0x2E - 'c'		*/	'c',		'C',		0,
/* 0x2F - 'v'		*/	'v',		'V',		0,
/* 0x30 - 'b'		*/	'b',		'B',		0,
/* 0x31 - 'n'		*/	'n',		'N',		0,
/* 0x32 - 'm'		*/	'm',		'M',		0,
/* 0x33 - ','		*/	',',		'<',		0,
/* 0x34 - '.'		*/	'.',		'>',		0,
/* 0x35 - '/'		*/	'/',		'?',		PAD_SLASH,
/* 0x36 - r. SHIFT	*/	SHIFT_R,	SHIFT_R,	0,
/* 0x37 - '*'		*/	'*',		'*',    	0,
/* 0x38 - ALT		*/	ALT_L,		ALT_L,  	ALT_R,
/* 0x39 - ' '		*/	' ',		' ',		0,
/* 0x3A - CapsLock	*/	CAPS_LOCK,	CAPS_LOCK,	0,
/* 0x3B - F1		*/	F1,		F1,		0,
/* 0x3C - F2		*/	F2,		F2,		0,
/* 0x3D - F3		*/	F3,		F3,		0,
/* 0x3E - F4		*/	F4,		F4,		0,
/* 0x3F - F5		*/	F5,		F5,		0,
/* 0x40 - F6		*/	F6,		F6,		0,
/* 0x41 - F7		*/	F7,		F7,		0,
/* 0x42 - F8		*/	F8,		F8,		0,
/* 0x43 - F9		*/	F9,		F9,		0,
/* 0x44 - F10		*/	F10,		F10,		0,
/* 0x45 - NumLock	*/	NUM_LOCK,	NUM_LOCK,	0,
/* 0x46 - ScrLock	*/	SCROLL_LOCK,	SCROLL_LOCK,	0,
/* 0x47 - Home		*/	'7',	'7',		HOME,
/* 0x48 - CurUp		*/	UP,		UP,		    UP,
/* 0x49 - PgUp		*/	'9',	'9',		PAGEUP,
/* 0x4A - '-'		*/	'-',	'-',		0,
/* 0x4B - Left		*/	LEFT,	LEFT,		LEFT,
/* 0x4C - MID		*/	'5',	'5',		0,
/* 0x4D - Right		*/	RIGHT,	RIGHT,		RIGHT,
/* 0x4E - '+'		*/	'+',	'+',		0,
/* 0x4F - End		*/	'1',	'1',		END,
/* 0x50 - Down		*/	DOWN,	DOWN,		DOWN,
/* 0x51 - PgDown	*/	'3',	'3',		PAGEDOWN,
/* 0x52 - Insert	*/	'0',	'0',		INSERT,
/* 0x53 - Delete	*/	'.'    ,	'.',		DELETE,
/* 0x54 - Enter		*/	0,		0,		0,
/* 0x55 - ???		*/	0,		0,		0,
/* 0x56 - ???		*/	0,		0,		0,
/* 0x57 - F11		*/	F11,		F11,		0,
/* 0x58 - F12		*/	F12,		F12,		0,
/* 0x59 - ???		*/	0,		0,		0,
/* 0x5A - ???		*/	0,		0,		0,
/* 0x5B - ???		*/	0,		0,		GUI_L,
/* 0x5C - ???		*/	0,		0,		GUI_R,
/* 0x5D - ???		*/	0,		0,		APPS,
/* 0x5E - ???		*/	0,		0,		0,
/* 0x5F - ???		*/	0,		0,		0,
/* 0x60 - ???		*/	0,		0,		0,
/* 0x61 - ???		*/	0,		0,		0,
/* 0x62 - ???		*/	0,		0,		0,
/* 0x63 - ???		*/	0,		0,		0,
/* 0x64 - ???		*/	0,		0,		0,
/* 0x65 - ???		*/	0,		0,		0,
/* 0x66 - ???		*/	0,		0,		0,
/* 0x67 - ???		*/	0,		0,		0,
/* 0x68 - ???		*/	0,		0,		0,
/* 0x69 - ???		*/	0,		0,		0,
/* 0x6A - ???		*/	0,		0,		0,
/* 0x6B - ???		*/	0,		0,		0,
/* 0x6C - ???		*/	0,		0,		0,
/* 0x6D - ???		*/	0,		0,		0,
/* 0x6E - ???		*/	0,		0,		0,
/* 0x6F - ???		*/	0,		0,		0,
/* 0x70 - ???		*/	0,		0,		0,
/* 0x71 - ???		*/	0,		0,		0,
/* 0x72 - ???		*/	0,		0,		0,
/* 0x73 - ???		*/	0,		0,		0,
/* 0x74 - ???		*/	0,		0,		0,
/* 0x75 - ???		*/	0,		0,		0,
/* 0x76 - ???		*/	0,		0,		0,
/* 0x77 - ???		*/	0,		0,		0,
/* 0x78 - ???		*/	0,		0,		0,
/* 0x78 - ???		*/	0,		0,		0,
/* 0x7A - ???		*/	0,		0,		0,
/* 0x7B - ???		*/	0,		0,		0,
/* 0x7C - ???		*/	0,		0,		0,
/* 0x7D - ???		*/	0,		0,		0,
/* 0x7E - ???		*/	0,		0,		0,
/* 0x7F - ???		*/	0,		0,		0
};


/*
	回车键:	把光标移到第一列
	换行键:	把光标前进到下一行
*/


/*====================================================================================*
				Appendix: Scan code set 1
 *====================================================================================*

KEY	MAKE	BREAK	-----	KEY	MAKE	BREAK	-----	KEY	MAKE	BREAK
--------------------------------------------------------------------------------------
A	1E	9E		9	0A	8A		[	1A	9A
B	30	B0		`	29	89		INSERT	E0,52	E0,D2
C	2E	AE		-	0C	8C		HOME	E0,47	E0,C7
D	20	A0		=	0D	8D		PG UP	E0,49	E0,C9
E	12	92		\	2B	AB		DELETE	E0,53	E0,D3
F	21	A1		BKSP	0E	8E		END	E0,4F	E0,CF
G	22	A2		SPACE	39	B9		PG DN	E0,51	E0,D1
H	23	A3		TAB	0F	8F		U ARROW	E0,48	E0,C8
I	17	97		CAPS	3A	BA		L ARROW	E0,4B	E0,CB
J	24	A4		L SHFT	2A	AA		D ARROW	E0,50	E0,D0
K	25	A5		L CTRL	1D	9D		R ARROW	E0,4D	E0,CD
L	26	A6		L GUI	E0,5B	E0,DB		NUM	45	C5
M	32	B2		L ALT	38	B8		KP /	E0,35	E0,B5
N	31	B1		R SHFT	36	B6		KP *	37	B7
O	18	98		R CTRL	E0,1D	E0,9D		KP -	4A	CA
P	19	99		R GUI	E0,5C	E0,DC		KP +	4E	CE
Q	10	19		R ALT	E0,38	E0,B8		KP EN	E0,1C	E0,9C
R	13	93		APPS	E0,5D	E0,DD		KP .	53	D3
S	1F	9F		ENTER	1C	9C		KP 0	52	D2
T	14	94		ESC	01	81		KP 1	4F	CF
U	16	96		F1	3B	BB		KP 2	50	D0
V	2F	AF		F2	3C	BC		KP 3	51	D1
W	11	91		F3	3D	BD		KP 4	4B	CB
X	2D	AD		F4	3E	BE		KP 5	4C	CC
Y	15	95		F5	3F	BF		KP 6	4D	CD
Z	2C	AC		F6	40	C0		KP 7	47	C7
0	0B	8B		F7	41	C1		KP 8	48	C8
1	02	82		F8	42	C2		KP 9	49	C9
2	03	83		F9	43	C3		]	1B	9B
3	04	84		F10	44	C4		;	27	A7
4	05	85		F11	57	D7		'	28	A8
5	06	86		F12	58	D8		,	33	B3

6	07	87		PRTSCRN	E0,2A	E0,B7		.	34	B4
					E0,37	E0,AA

7	08	88		SCROLL	46	C6		/	35	B5

8	09	89		PAUSE E1,1D,45	-NONE-
				      E1,9D,C5


-----------------
ACPI Scan Codes:
-------------------------------------------
Key		Make Code	Break Code
-------------------------------------------
Power		E0, 5E		E0, DE
Sleep		E0, 5F		E0, DF
Wake		E0, 63		E0, E3


-------------------------------
Windows Multimedia Scan Codes:
-------------------------------------------
Key		Make Code	Break Code
-------------------------------------------
Next Track	E0, 19		E0, 99
Previous Track	E0, 10		E0, 90
Stop		E0, 24		E0, A4
Play/Pause	E0, 22		E0, A2
Mute		E0, 20		E0, A0
Volume Up	E0, 30		E0, B0
Volume Down	E0, 2E		E0, AE
Media Select	E0, 6D		E0, ED
E-Mail		E0, 6C		E0, EC
Calculator	E0, 21		E0, A1
My Computer	E0, 6B		E0, EB
WWW Search	E0, 65		E0, E5
WWW Home	E0, 32		E0, B2
WWW Back	E0, 6A		E0, EA
WWW Forward	E0, 69		E0, E9
WWW Stop	E0, 68		E0, E8
WWW Refresh	E0, 67		E0, E7
WWW Favorites	E0, 66		E0, E6

*=====================================================================================*/
static struct irq_action kb_irq_action;
static struct tty ttys[MAX_TTY_NUM];
static struct tty * cur_tty;

static int alt_l;
static int alt_r;
static int ctrl_l;
static int ctrl_r;
static int shift_l;
static int shift_r;
static int capslock;
static int tab_wd = 4;

static void tty_update_base()
{
  vga_set_base(cur_tty->base + cur_tty->start_row * VGA_COL_NUM * VGA_CHAR_SIZE);
}
static void tty_update_cursor()
{
  vga_set_cursor(cur_tty->cur_col, cur_tty->cur_row - cur_tty->start_row);
}

static void tty_change_to(int tty_num)
{

  if (tty_num < 0 || tty_num >= MAX_TTY_NUM || !ttys[tty_num].wait_queue)
    return ;
  if (cur_tty == ttys + tty_num)
    return ;
  cur_tty = ttys + tty_num;
  tty_update_base();
  tty_update_cursor();
}

static void tty_do_clean(char *buffer, int count, char color)
{
  unsigned int i;
  for (i = 0; i < count; i++) {
    buffer[i << 1] = ' ';
    buffer[ (i << 1) + 1] = color;
  }
}

void kb_irq_handler(void *private, struct pt_regs * regs)
{
  uint8 code = read_keyboard();
  uint8 make = 0;
  uint32 * key;
  struct task_wait_entry * wait_entry;
  struct task_wait_queue * wait_queue;
  int col;
  int tty_num;
  if (!cur_tty || !cur_tty->wait_queue)
    return ;
  //deal code
  col = 0;
  if (code == 0xE1){
  }else if (code == 0xE0){
  }else{
    make = (code & FLAG_BREAK ? 0 : 1);
    key = keymap + ((code & 0x7F) * MAP_COLS);
    col = 0;

    if (shift_l || shift_r)
      col = !col;
    if (capslock)
      col = !col;

    //take with spacial code
    switch(key[col]){
    case SHIFT_L:
      shift_l = make;
      return ;

    case SHIFT_R:
      shift_r = make;
      return ;

    case CTRL_L:
      ctrl_l = make;
      return ;

    case CTRL_R:
      ctrl_r = make;
      return ;

    case ALT_L:
      alt_l = make;
      return ;

    case ALT_R:
      alt_r = make;
      return ;

    case CAPS_LOCK:
      if (make)
        capslock = !capslock;
      return ;

    case F1:
    case F2:
    case F3:
    case F4:
    case F5:
    case F6:
    case F7:
    case F8:
    case F9:
    case F10:
    case F11:
    case F12:
      if (make){
        tty_num = key[col] - F1;
        if (ctrl_l || ctrl_r)
          tty_change_to(tty_num);
      }
      return ;
    case UP:
      if (make && (ctrl_l || ctrl_r)){
        if (cur_tty->start_row){
          cur_tty->start_row--;
          tty_update_base();
        }
      }
      return ;
    case DOWN:
      if (make && (ctrl_l || ctrl_r)){
        if (cur_tty->start_row <= cur_tty->cur_row - VGA_ROW_NUM){
          cur_tty->start_row++;
          tty_update_base();
        }
      }
      return ;
    }

    //take with  normal char
    if (make){

      //signal
      if ((ctrl_l || ctrl_r) &&
          (key[col] == 'c' || key[col] == 'C')){
        sig_send(cur_tty->creater, SIGINT);
        return ;
      }
      if ((ctrl_l || ctrl_r) &&
          key[col] == '\\'){
        sig_send(cur_tty->creater, SIGQUIT);
        return ;
      }

      //normal key
      wait_queue = cur_tty->wait_queue;
      if (list_empty(&(wait_queue->entry_list)))
        return ;
      wait_entry = container_of(wait_queue->entry_list.next,
                              struct task_wait_entry,
                              wait_list_entry);
      if ((ctrl_l || ctrl_r) &&
          (key[col] == 'd' || key[col] == 'D')){
        wait_entry->private = (void*)-1; //EOF
      }
      else
        wait_entry->private = key[col];
      task_notify_one(wait_queue);
    }
  }
}

static void tty_putc(char c, struct tty * tty)
{
  char *base;
  int offset;

  if (!tty)
    return ;
  base = tty->base;
  if (c == '\n'){
    tty->cur_col = 0;
    tty->cur_row++;
    tty_do_clean(base + tty->cur_row * VGA_COL_NUM * VGA_CHAR_SIZE, VGA_COL_NUM, tty->cur_color);
  }else if (c == '\r')
    tty->cur_col = 0;
  else if (c == '\t')
    tty->cur_col = (tty->cur_col + tab_wd) / tab_wd * tab_wd;
  else if (c == '\b'){
    if (tty->cur_col == 0)
      return ;
    tty->cur_col--;
    offset = tty->cur_col + tty->cur_row * VGA_COL_NUM;
    base[offset << 1] = c;
    base[(offset << 1) + 1] = tty->cur_color;

    if (tty == cur_tty && cur_tty->cur_row - cur_tty->start_row < VGA_ROW_NUM)
      vga_putc(' ', tty->cur_color, tty->cur_col, tty->cur_row - tty->start_row);

  }else{

    offset = tty->cur_col + tty->cur_row * VGA_COL_NUM;
    base[offset << 1] = c;
    base[(offset << 1) + 1] = tty->cur_color;

    if (tty == cur_tty && cur_tty->cur_row - cur_tty->start_row < VGA_ROW_NUM)
      vga_putc(c, tty->cur_color, tty->cur_col, tty->cur_row - tty->start_row);

    tty->cur_col++;
  }

  if (tty->cur_col >= VGA_COL_NUM){
    tty->cur_row++;
    tty->cur_col = 0;
    tty_do_clean(base + tty->cur_row * VGA_COL_NUM * VGA_CHAR_SIZE, VGA_COL_NUM, tty->cur_color);
  }

  //if we realy get the end of buffer, we must back to the begaing
  if (tty->cur_row == TTY_ROW_NUM){
    memcpy(tty->base, tty->base + TTY_BASE_SIZE - TTY_SCREEN_BUFFER_SIZE, TTY_SCREEN_BUFFER_SIZE);
    tty->start_row = 1;
    tty->cur_row = VGA_ROW_NUM;
    tty_do_clean(base + tty->cur_row * VGA_COL_NUM * VGA_CHAR_SIZE, VGA_COL_NUM, tty->cur_color);
    if (tty == cur_tty)
      tty_update_base();
  }
  //auto scroll
  else if (tty->cur_row >= tty->start_row + VGA_ROW_NUM){
    tty->start_row = tty->cur_row - VGA_ROW_NUM + 1;
    if (tty == cur_tty)
      tty_update_base();
  }

  if (tty == cur_tty)
    tty_update_cursor();
}

int tty_read_input(char * buffer, unsigned long len)
{
  struct task * task = task_get_cur();
  int tty_num = task->tty_num;

  if (tty_num < 0 || tty_num >= MAX_TTY_NUM)
    return -EINVAL;

  struct tty * tty = ttys + tty_num;
  struct task_wait_queue * wait_queue = tty->wait_queue;
  if (!wait_queue)
    return -ENOTTY;

  //only fg_task can read from tty
  if (task != tty->fg_task){
    task_block(task);
    task_schedule();
    if (sig_is_pending(task))
      return -EINTR;
  }
  if (!list_empty(&(wait_queue->entry_list)))
    return -EBUSY;

  struct task_wait_entry entry;
  entry.task = task;
  entry.wake_up = task_gener_wake_up;
  task_wait_on(&entry, wait_queue);

  unsigned long i;
  for (i = 0 ;i < len;){
    task_block(task);
    task_schedule();

    if (sig_is_pending(task)){
      task_leave_from_wq(&entry);
      return -EINTR;
    }

    int input = (int)entry.private;
    if (input == -1)
      break;
    else if (input == '\t'){
      do{
        buffer[i++] = ' ';
        tty_putc(' ', tty);
      }while ((i % tab_wd));
      continue;
    }
    if (input == '\b'){
      if (i > 0){
        i--;
        tty_putc('\b', tty);
      }
      continue;
    }

    buffer[i] = input;
    tty_putc(input, tty);
    i++;
    //line buffer
    if (input  == '\n')
      break;

  }
  task_leave_from_wq(&entry);

  return i;
}



void tty_set_cursor(int x, int y)
{
  int tty_num = task_get_cur()->tty_num;
  if (tty_num < 0 || tty_num >= MAX_TTY_NUM)
    return ;
  struct tty * tty = ttys + tty_num;
  if (!tty->wait_queue)
    return ;

  tty->cur_col = x;
  tty->cur_row = tty->start_row + y;

  if (tty == cur_tty)
    vga_set_cursor(x, y);
}


void tty_get_cursor(int *x, int *y)
{
  int tty_num = task_get_cur()->tty_num;
  if (tty_num < 0 || tty_num >= MAX_TTY_NUM)
    return ;
  struct tty * tty = ttys + tty_num;
  if (!tty->wait_queue)
    return ;

  *x = tty->cur_col;
  *y = tty->cur_row - tty->start_row;
}


void tty_clear()
{
  int tty_num = task_get_cur()->tty_num;
  if (tty_num < 0 || tty_num >= MAX_TTY_NUM)
    return ;
  struct tty * tty = ttys + tty_num;
  if (!tty->wait_queue)
    return ;

  tty->start_row = 0;
  tty->cur_col = 0;
  tty->cur_row = 0;

  tty_do_clean(tty->base, VGA_COL_NUM * VGA_ROW_NUM, tty->cur_color);

  if (tty == cur_tty){
    tty_update_base();
    tty_update_cursor();
  }
}


void tty_init()
{
  irq_action_init(&kb_irq_action);
  kb_irq_action.action = kb_irq_handler;
  irq_regist(0x21, &kb_irq_action);
}

int tty_open_new(struct task * task)
{
  int i;
  for (i = 0; i < MAX_TTY_NUM; i++){
    if (!ttys[i].base){
      memset(ttys + i, 0, sizeof(struct tty));
      ttys[i].base = mm_kmalloc(TTY_BASE_SIZE);
      if (!ttys[i].base)
        return -1;

      ttys[i].wait_queue = task_new_wait_queue();
      if (!ttys[i].wait_queue){
        mm_kfree(ttys[i].base);
        return -1;
      }
      tty_do_clean(ttys[i].base, VGA_COL_NUM * VGA_ROW_NUM, 0x7);
      if (!cur_tty){
        cur_tty = ttys + i;
        tty_update_base();
        tty_update_cursor();
      }
      ttys[i].creater = task;
      ttys[i].fg_task = task;
      task->tty_num = i;
      return 0;
    }
  }
  return -1;
}

void putc(char c)
{
  int tty_num = task_get_cur()->tty_num;
  if (tty_num < 0 || tty_num >= MAX_TTY_NUM)
    return ;

  struct tty * tty = ttys + tty_num;
  if (!tty->base)
    return ;
  tty_putc(c, tty);
}


void tty_set_color(char color)
{
  int tty_num = task_get_cur()->tty_num;
  if (tty_num < 0 || tty_num >= MAX_TTY_NUM)
    return ;
  struct tty * tty = ttys + tty_num;
  if (!tty->base)
    return ;
  tty->cur_color = color;
}

int tty_set_fg_task(int tty_num, struct task * onwer_task, struct task * target_task)
{
  if (tty_num < 0 || tty_num >= MAX_TTY_NUM || !ttys[tty_num].base)
    return -EINVAL;
  if (onwer_task != ttys[tty_num].creater)
    return -EPERM;
  ttys[tty_num].fg_task = target_task;
  return 0;
}
