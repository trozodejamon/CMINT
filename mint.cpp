/********************************************************************************
 * MINT Main Source
 * Jason CJ Tay
 * TODO: [] arrays, early loop exit
 * int: 4 bytes, long: 4 bytes, char*: 4 bytes
 ********************************************************************************/
#include "mint.h"

const char strPrompt[5] = "\n\r> ";
const char strEndLine[3] = "\n\r";
const char strTicksTaken[8] = "Ticks: ";
const char hexChars[17] = "0123456789ABCDEF";
const char strEmptyStack[8] = "<Empty>";
const char strError[6] = "Error";
const char strRStackFull[21] = "Return stack is full";
char inbuf[INBUF_MAX];
char execBuf[EXBUF_SIZE];
BYTE execNum;
CELL_T execLast;
char *execPtrs[26];            // One for each uppercase letter
CELL_T dstack[MINT_STACK_SIZE];
volatile INT8 tos, ll;        // Top of stack, loop-level
BYTE numMode;                 // Numeric mode for input/output
char* wordIndex[26];          // Array of pointers to strings allocated on the heap
CELL_T vars[26];
CELL_T sysvars[26];
unsigned int ti;              // Temporary integer storage
char* rStack[LOOP_MAX];
CELL_T llimit[LOOP_MAX];
CELL_T li[LOOP_MAX];    // Loop iterator storage

// TODO: You must implement these functions in your platform specific main.c
extern int available(void);
extern void txChar(char);
extern BYTE rxChar(void);
extern CELL_T getMillis(void);

/**
  * @brief  Transmit a string of len characters via UART2.
  * @retval None
  */
//txStr
void printStr(char *str, int len)
{
  if((str==NULL) || (len < 1) || (len > 255))
    return;
  
  int idx;
  char outchar;
  idx = 0;
  while(((outchar=str[idx])!=NULL) && (idx<len))
  {
    txChar(outchar);
    idx++;
  }
}

/**
  * @brief  Convert and transmit Decimal ASCII representation via UART.
  * @retval None
  */
void printDec(int val)
{
  int cc, _val, remain, isNeg, maxc;
#ifdef __64bit__
  char buf[21] = " 0000000000000000000";
  cc = 19;  // point to the last usable element in the buffer
  maxc=20;
#elif defined(__16bit__)
  char buf[7] = " 00000";
  cc = 5;  // point to the last usable element in the buffer
  maxc=6;
#else
  char buf[13] = " 00000000000";
  cc = 11;  // point to the last usable element in the buffer
  maxc=12;
#endif
  
  if(val != 0)
  {
    _val = val;
    
    isNeg = _val < 0;
    
    if(isNeg) _val = -_val;
    while((cc > 0) && (_val > 0))
    {
      remain = _val % 10;
      _val = _val / 10;
      buf[cc] += remain;
      cc--;
    }
    if(isNeg) buf[cc] = '-';
    else cc++;
  }
  
  printStr(buf+cc, maxc-cc);
}

/**
  * @brief  Convert and transmit HEX ASCII representation via UART.
  * @retval None
  */
void printHex(int val, int padZero)
{
  int cc = 0, nibc, nibv, maxc;
#ifdef __64bit__
  char buf[17] = "0000000000000000";
  maxc = 60;
#else
  char buf[9] = "00000000";
  if(sizeof(CELL_T) == 2) maxc = 12;
  else maxc = 28;
#endif
  
  if(val == 0)
  {
    cc = 1;
  }
  else
  {
    for(nibc=maxc; nibc>-1; nibc-=4)
    {
      nibv = (val >> nibc) & 0xf;
      if(nibv == 0)
      {
        if(padZero) cc++; // Yes, we have a printable nibble.
      }
      else
      {
        buf[cc]=hexChars[nibv];
        cc++;
        padZero=1;    // Make sure we output the remaining zeroes
      }
    }
  }
  printStr(buf, cc);
}

/**
  * @brief  Print and endline sequence (MS-DOS 0x0D, 0x0A style)
  * @retval None
  */
void printEndLine()
{
  printStr((char*)strEndLine, 2);
}

/** 
  * @brief  Displays the standard prompt
  * @retval None
  */
void prompt()
{
  printStr((char *)strPrompt, 4);
}

/**
  * @brief  Zeroes out the Data Stack completely.
  * @retval None
  */
void clearDStack()
{
  int i;
  for(i=0; i<MINT_STACK_SIZE; i++) dstack[i] = 0;
  tos=-1;   // Empty stack
}

/**
  * @brief  Zeroes out the Data Stack completely.
  * @retval None
  */
void clearInbuf()
{
  int cc;
  for(cc=0; cc<INBUF_MAX; cc++) inbuf[cc] = 0;
}

/**
  * @brief  Implements the Forth .s word.
  * @retval None
  */
void printDStack()
{
  char sp = ' ';
  BYTE i;
  printStr("=> ", 3);
  if(tos < 0)
    printStr((char*)strEmptyStack, 7);
  else
    for(i=0; i<=tos; i++)
    {
      if(numMode == NUMODE_DEC)
        printDec(dstack[i]);
      else
        printHex(dstack[i], 0);
      printStr(&sp, 1);
    }
  printEndLine();
}

/**
  * @brief  Display user defined words.
  * @retval None
  * Dump the user dictionary to the display, MINT style, which includes
  * printing the entire definition as well.
  */
void printUserDict()
{
  int i;
  char *cp;
  printEndLine();
  for(i=0; i<execNum; i++)
  {
    txChar(':');
    txChar('A' + i);
    cp = execPtrs[i];
    while(*cp)
    {
      txChar(*cp);
      if(*cp == 13) txChar(10);
      cp++;
    }
    printEndLine();
  }
}

/** 
  * @brief  Simple single line editing function.
  * @retval int
  * This version has been edited to be MINT aware and to balance brackets () and [].
  * Nesting allowed to 8 levels for () and no nesting allowed for [].
  */
BYTE editLine()
{
  BYTE inchar;
  BYTE cc;
  BYTE loopLvl;   // 0=None, 1=1 level, 2=nested once, etc., up to LOOP_MAX
  BYTE error;
  BYTE inArray;
  BYTE inStringLiteral;
  BYTE entryEnded;
  BYTE inDef;     // It is a word definition.

  inchar=0;
  cc = 0;
  loopLvl = 0;
  error = 0;
  inArray = 0;
  inStringLiteral = 0;
  entryEnded = 0;
  inbuf[cc] = 0;
  inDef = 0;
  while(!entryEnded && !error)
  {
    if(available() && (cc < INBUF_MAX-1))   // See if a char has been received into the RDR
    {
      inchar = rxChar();                                  // Read the char
      if(inchar < 128)
      {
        switch(inchar)
        {
          case KEY_ENTER:
            if((loopLvl<1) && !inArray && !inDef)
            {
              // Line has been completed.
              inbuf[cc<INBUF_MAX-1?cc:INBUF_MAX-1]=0;
              printEndLine();
              entryEnded = 1;
            }
            else
            {
              // Add in the newline like any other char
              if(cc<INBUF_MAX-1)    // This will just fail silently if not enough space.
              {
                inbuf[cc] = inchar;
                printEndLine();
                cc++;
              }
            }
            break;
          case KEY_CTRLP:
            printDStack();
            entryEnded = 1;
            break;
          case KEY_LISTDEFS:
            printUserDict();
            entryEnded = 1;
            break;
          case KEY_DELETE:
            if(cc>0)
            { // Echo the delete to the VT100 terminal and update our internal buffer correctly.
              printStr((char*)&inchar, 1);
              cc--;
              switch(inbuf[cc])
              {
                case '(':
                  loopLvl--;
                  break;
                case ')':
                  loopLvl++;
                  break;
                case '[':
                  inArray = 0;
                  break;
                case ']':
                  inArray = 1;
                  break;
                case ':':
                  if(!inStringLiteral) inDef = 0;
                  break;
                case ';':
                  if(!inStringLiteral) inDef = 1;
                  break;
              }
              inbuf[cc] = 0;
            }
            break;
          default:
            if((inchar >= 32) && (inchar <= 127))
            {
              if(cc<INBUF_MAX-1)
              {
                switch(inchar)
                {
                  case '(':
                    if(loopLvl<LOOP_MAX) loopLvl++;
                    else error = 1;
                    break;
                  case ')':
                    if(loopLvl>0) loopLvl--;
                    else error = 1;
                    break;
                  case '[':
                    inArray = 1;
                    break;
                  case ']':
                    inArray = 0;
                    break;
                  case ':':
                    if(!inStringLiteral) inDef = 1;
                    break;
                  case ';':
                    if(!inStringLiteral) inDef = 0;
                    break;
                  case '`':
                    inStringLiteral = inStringLiteral?0:1;
                    break;
                }
                inbuf[cc] = inchar;
                printStr((char*)&inchar, 1);
                cc++;
              }
            } // default in switch
        } // switch()
      } // if ASCII
    } // If char received
  } // while not entry ended yet
  if(error)
  {
    printStr("Error during line input\n\r", 26);
    clearInbuf();
  }
  return cc;
}

//*****************************************************************************
// Push and Pop, stack operations explainer
// 
//*****************************************************************************

/**
  * @brief  Push an address onto the Return Stack.
  * @retval int
  */
int pushR(CELL_T val)
{
  return 0;
}

/**
  * @brief  Pop an address off the Return Stack.
  * @retval int
  */
int popR(CELL_T *val)
{
  return 0;
}

/**
  * @brief  Push an value onto the Data Stack.
  * @retval int
  */
int push(CELL_T val)
{
  int retval = ERR_NONE;
  if(MINT_STACK_SIZE-tos>=2)    // Figure out if we have space for the item. If not overflow.
  {
    tos++;
    dstack[tos] = val;
  } else retval = ERR_OVERFLOW;
  return retval;
}

/**
  * @brief  Used to remove one stack element.
  * @retval int
  */
int drop1(void)
{
  int retval = ERR_NONE;
  if(tos > -1)
  {
    dstack[tos] = 0;    // Clear top of stack.
    tos--;              // Decrement. If 1 item left, it would have been 0-1=-1, so empty
  } else retval = ERR_UNDERFLOW;
  return retval;
}

/**
  * @brief  Used to quickly drop more than one stack element in one go, e.g., after multi operand operations
  * @retval int
  */
int dropn(int n)
{
  int retval = ERR_NONE;
  BYTE i;
  if(tos >= (n-1))
  {
    for(i=0; i<n; i++)
      dstack[tos-i] = 0;
    tos -= n;
  } else retval = ERR_BAD_PARAM;
  return retval;
}

/**
  * @brief  The MINT interpreter loop.
  * @retval None
  */
void interpret(char *pr)
{
  int retval = ERR_NONE;
  char *str;
  int *iptr;
  while(*pr)
  {
    switch(*pr)
    {
      case KEY_ENTER:
      case ' ':     // NOP
        retval = ERR_NONE;
        pr++;
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        ti = 0;
        while((*pr >= '0') && (*pr <= '9'))
        {
          ti = (10*ti) + (*pr - '0');
          pr++;
        }
        retval = push(ti);
        break;
      case '!':     // STORE a value to memory
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
        {
          iptr = (int*)dstack[tos];
          *iptr = dstack[tos-1];
          retval = dropn(2);
        }
        pr++;
        break;
      case '@':     // FETCH a value from memory
        if(tos < 0) retval = ERR_UNDERFLOW;
        else
        {
          iptr = (int*)dstack[tos];
          dstack[tos] = *iptr;
          retval = ERR_NONE;
        }
        pr++;
        break;
      case '"':     // DUP
        retval = push(dstack[tos]);
        pr++;
        break;
      case '#':     // Following number is hexadecimal
        ti = 0;
        pr++;        // Move to the char after the '#'
        while(((*pr >= '0') && (*pr <= '9')) || ((*pr >= 'a') && (*pr <= 'f')) || ((*pr >= 'A') && (*pr <= 'F')))
        {
          ti <<= 4;                             // Shift by 1 hex digit
          if(*pr <= '9') ti |= *pr - '0';       // Numerals
          else if(*pr >= 'a') ti |= *pr - 87;   // Lower case a-f
          else ti |= *pr - 55;                  // Upper case A-F
          pr++;
        }
        retval = push(ti);
        break;
      case '$':     // SWAP
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
        {
          retval = ERR_NONE;
          ti = dstack[tos];
          dstack[tos] = dstack[tos-1];
          dstack[tos-1] = ti;
        }
        pr++;
        break;
      case '%':     // OVER
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
          retval = push(dstack[tos-1]);
        pr++;
        break;
      case '~':     // ~ Forth ROT
        if(tos < 2) retval = ERR_UNDERFLOW;
        else
        {
          ti = dstack[tos-2];             // Save 3rd element down.
          dstack[tos-2] = dstack[tos-1];  // Move 2nd down to 3rd.
          dstack[tos-1] = dstack[tos];    // Move 1st down to 2nd.
          dstack[tos] = ti;               // Place previous 3rd on top.
          retval = ERR_NONE;
          pr++;
        }
        break;
      case '&':     // Bitwise AND
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
        {
          dstack[tos-1] = dstack[tos-1] & dstack[tos];
          retval = drop1();
        }
        pr++;
        break;
      case 39:      // Drop - apostrophe '
        retval = drop1();
        pr++;
        break;
      case '(':     // BEGIN a loop or IF-THEN-ELSE
        if(tos < 0) retval = ERR_UNDERFLOW;
        else if(dstack[tos] == 0)       // 0 loop or condition failed, so either way, skip
        {
          // Scan forward to find the matching closing ')'.
          // Track and match nested brackets.
          drop1();
          ll++;     // Must account for this '('
          ti = ll;  // Save the current loop level
          do
          {
            pr++;
            switch(*pr)
            {
              case '(':
                ll++;
                break;
              case ')':
                ll--;
                break;
            }
          } while((*pr != ')') && (ll >= ti));
          pr++;
          if(*pr == '(')        // There's an else condition
            retval = push(1);   // Set it up to execute once
          else
            retval = ERR_NONE;
        }
        else
        {
          pr++;       // Skip to the next char
          if(*pr && (ll<LOOP_MAX-1))
          {
            ll++;                       // Advance to the next loop level
            li[ll] = 0;                 // Reset the loop iterator
            llimit[ll] = dstack[tos];   // Loop max tracking
            rStack[ll] = pr;            // Save the repeat address
            retval = drop1();           // Remove the tos
          }
          else
            retval = ERR_BAD_PARAM;
        }
        break;
      case ')':           // END a loop or conditionally executed code block
        retval = ERR_NONE;
        if(ll>-1)
        {
          li[ll]++;       // Increment the loop counter for the next round.
          if(li[ll] < llimit[ll]) // Still looping
          {
            pr = rStack[ll];  // Jump to head of the loop
          }
          else            // We've reached the end of the loop
          {
            // See if it is an else clause after an executed IF clause
            if((*(pr+1) == '(') && (llimit[ll] == 1))
            {
              retval = push(0);   // Stop the false ELSE clause from executing
            }
            ll--;         // Drop down a level
            pr++;         // Step to the next char
          }
        }
        else
          retval = ERR_UNDERFLOW;
        break;
      case '<':     // Comparison LT
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
        {
          dstack[tos-1] = dstack[tos-1]<dstack[tos]?1:0;
          retval = drop1();
        }
        pr++;
        break;
      case '=':     // Comparison EQ
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
        {
          dstack[tos-1] = dstack[tos-1]==dstack[tos]?1:0;
          retval = drop1();
        }
        pr++;
        break;
      case '>':     // Comparison GT
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
        {
          dstack[tos-1] = dstack[tos-1]>dstack[tos]?1:0;
          retval = drop1();
        }
        pr++;
        break;
      case '*':     // Multiply
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
        {
          dstack[tos-1] = dstack[tos-1] * dstack[tos];
          retval = drop1();
        }
        pr++;
        break;
      case '+':     // Add
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
        {
          dstack[tos-1] = dstack[tos-1] + dstack[tos];
          retval = drop1();
        }
        pr++;
        break;
      case ',':     // Print as hex
        if(tos<0) retval = ERR_UNDERFLOW;
        else
        {
          printHex(dstack[tos], 0);
          retval = drop1();
        }
        pr++;
        break;
      case '-':     // Subtract
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
        {
          dstack[tos-1] = dstack[tos-1] - dstack[tos];
          retval = drop1();
        }
        pr++;
        break;
      case '.':     // Print top of stack
        if(tos<0) retval = ERR_UNDERFLOW;
        else
        {
          printDec(dstack[tos]);
          retval = drop1();
        }
        pr++;
        break;
      case '/':     // Divide
        if(tos < 1) retval = ERR_UNDERFLOW;
        else if(dstack[tos] == 0) retval = ERR_BAD_PARAM;
        else
        {
          ti = dstack[tos-1];
          dstack[tos-1] = ti / dstack[tos];   // Quotient
          dstack[tos] = ti % dstack[tos];     // Remainder
          retval = ERR_NONE;
        }
        pr++;
        break;
      case '^':     // Bitwise XOR
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
        {
          dstack[tos-1] = dstack[tos-1] ^ dstack[tos];
          retval = drop1();
        }
        pr++;
        break;
      case '_':     // Negation NEG
        if(tos<0) retval = ERR_UNDERFLOW;
        else
        {
          retval = ERR_NONE;
          dstack[tos] = -dstack[tos];
        }
        pr++;
        break;
      case ':':     // Define a new 'word'
        pr++;       // Advance next char
        if((*pr >= 'A') && (*pr <= 'Z'))
        {
          ti = execLast+1;
          execPtrs[*pr-'A'] = execBuf+ti;
          pr++;
          while((*pr != ';') && (execLast < EXBUF_SIZE-3))    // Zero indexed, final null, and ';' return char
          {
            execBuf[ti] = *pr;
            pr++;
            ti++;
          }
          execBuf[ti] = ';';
          ti++;
          execBuf[ti] = 0;
          execLast = ti;
          execNum++;
          pr++;
          retval = ERR_NONE;
        }
        else
          retval = ERR_BAD_PARAM;
        break;
      case ';':
        // Return from a user defined word.
        if(ll>=0)
        {
          pr = rStack[ll];    // Retrieve the return address.
          ll--;               // Drop down a loop level.
          retval = ERR_NONE;
        }
        else
          retval = ERR_UNDERFLOW;
        break;
      case '?':
        while(!available()) ;       // Make sure there is a char available
        retval = push(rxChar());    // Push it onto the stack
        pr++;
        break;
    /*----- Handle defined words -----*/
      case 'A':
      case 'B':
      case 'C':
      case 'D':
      case 'E':
      case 'F':
      case 'G':
      case 'H':
      case 'I':
      case 'J':
      case 'K':
      case 'L':
      case 'M':
      case 'N':
      case 'O':
      case 'P':
      case 'Q':
      case 'R':
      case 'S':
      case 'T':
      case 'U':
      case 'V':
      case 'W':
      case 'X':
      case 'Y':
      case 'Z':
        // Add return address to the resturn Stack, then jump to execBuf[a] and execute from there.
        ti = *pr - 'A';
        if((ll<LOOP_MAX-1) && execPtrs[ti])
        {
          ll++;
          li[ll] = 0;
          llimit[ll] = 1;     // It's a user word, so just 1 go.
          rStack[ll] = pr+1;  // Save the address to return to.
          pr = execPtrs[ti];
          retval = ERR_NONE;
        }
        else
        {
          pr++;               // Skip to next
          retval = ERR_BAD_PARAM;
        }
        break;
      case '[':     // Begin an array definition
        pr++;
        break;
      case ']':     // End and array definition
        pr++;
        break;
      case '`':     // Print a string
        retval = ERR_NONE;
        ti = 0;     // Count the chars
        pr++;
        str = pr;   // String start
        // Keep going until we get to the end of the available chars or to a closing backtick.
        while(*pr && (*pr != '`')) { ti++; pr++; }
        if(ti > 0) printStr(str, ti);
        pr++;
        break;
      case '{':     // Left Shift 1 bit (2*)
        if(tos < 0) retval = ERR_UNDERFLOW;
        else
        {
          dstack[tos] <<= 1;
          retval = ERR_NONE;
        }
        pr++;
        break;
      case '|':     // Bitwise OR
        if(tos < 1) retval = ERR_UNDERFLOW;
        else
        {
          dstack[tos-1] = dstack[tos-1] | dstack[tos];
          retval = drop1();
        }
        pr++;
        break;
      case '}':     // Right shift 1 bit (2/)
        if(tos < 0) retval = ERR_UNDERFLOW;
        else
        {
          dstack[tos] >>= 1;
          retval = ERR_NONE;
        }
        pr++;
        break;
      /*----- Handle variables -----*/
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'f':
      case 'g':
      case 'h':
      case 'i':
      case 'j':
      case 'k':
      case 'l':
      case 'm':
      case 'n':
      case 'o':
      case 'p':
      case 'q':
      case 'r':
      case 's':
      case 't':
      case 'u':
      case 'v':
      case 'w':
      case 'x':
      case 'y':
      case 'z':
        retval = push((CELL_T)(vars+(*pr - 'a')));
        pr++;
        break;
      case 92:      // Escape character for extended codes '\' char
        pr++;       // Get the next character
        switch(*pr)
        {
          case 'i':     // Push the address of the loop iterator onto the stack
            if(ll>-1)
              retval = push((CELL_T)(li+ll));
            else
              retval = ERR_BAD_PARAM;
            pr++;
            break;
          case 'j':
            if(ll>0)
              retval = push((CELL_T)(li+ll-1));
            else
              retval = push(0);
            pr++;
            break;
          case ',':     // Emit TOS as a character
            if(tos < 0) retval = ERR_UNDERFLOW;
            else
            {
              txChar(dstack[tos]);
              pr++;
              retval = drop1();
            }
            break;
          case '$':     // Print newline
            retval = ERR_NONE;
            printEndLine();
            pr++;
            break;
          case 'm':
            retval = push(getMillis());
            pr++;
            break;
          case 'a':     // Data stack start variable
            retval = push((CELL_T)dstack);
            pr++;
            break;
          case 'b':     // Base16 flag variable
            retval = push((CELL_T)&numMode);
            pr++;
            break;
          case 'd':     // Start of user definitions
            retval = push((CELL_T)execBuf);
            pr++;
            break;
          case 'h':     // Heap pointer variable
            retval = ERR_NONE;
            pr++;
            break;
          case 92:      // Comment to newline or end of string
            while(*pr && (*pr != KEY_ENTER)) pr++;
            if(*pr == KEY_ENTER) pr++;
            retval = ERR_NONE;
            break;
          case '#':     // Extended-Hash \# operations
            pr++;
            switch(*pr)
            {
              case '3':     // Push the depth of the stack onto the stack
                retval = push(tos+1);
                pr++;
                break;
              case '4':     // Non-destructively print MINT stack
                printDStack();
                pr++;
                retval = ERR_NONE;
                break;
            }
            break;
          default:
            if((*pr >= 'a') && (*pr <= 'z'))
            { // System variables
              retval = push((CELL_T)(sysvars+(*pr - 'a')));
              pr++;
            }
        }
        break;
    }
    if(retval != ERR_NONE)
    {
      clearDStack();
      printStr((char*)strError, 5);
      printStr((char*)strEndLine, 2);
      retval = ERR_NONE;    // Reset the error variable
      break;  // Exit the while loop. Ignore the rest of the input.
    }
  } // while
  clearInbuf();
} // interpret()

void mintInit(void)
{
  for(ti=0; ti<26; ti++)     // Initialise variables
  {
    vars[ti] = 0;
    sysvars[ti] = 0;
  }
  printEndLine();
  numMode = NUMODE_DEC;
  ll = -1;    // Reset the loop level. -1 means no loops
  for(ti=0; ti<LOOP_MAX; ti++)
  {
    li[ti] = 0;
    rStack[ti] = 0;
    llimit[ti] = 0;
  }
  clearDStack();
  execNum = 0;
  execLast = -1;
  for(ti=0; ti<26; ti++) execPtrs[ti] = 0;
  printStr("MINT 1.1", 8);
//  printStr("\n\rInt: ", 7);
//  printDec(sizeof(CELL_T));
//  printStr(" Ptr: ", 6);
//  printDec(sizeof(CELL_T*));
}

void mintRun(void)
{
    // Wait for input char.
    prompt();
    editLine();
    // Decode chars.
    interpret(inbuf);
}
