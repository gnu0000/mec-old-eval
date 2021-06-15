/*
 * Craig Fitzgerald
 */

#include <os2.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "eval.h"


BIG _Eval (PPSZ ppszExp, USHORT uLevel);
BOOL  ValidBIG (BIG bgNum);
void  InvalidateBIG (PBIG pbgNum);
PBIG  AToBIG (PBIG pbig, PSZ psz);
PSZ   Fmat (BIG bgNum, USHORT uFmt);
BIG   SetVar (USHORT uIndex, BIG big);


typedef BIG    (*PMTHFN) (BIG big);

//
// This structure is used to store
// Function names and pointers for 
// function calls in expressions
//
typedef struct
   {
   PSZ     pszFn;
   PMTHFN  pfn;
   } FNDESCR;

//
// This structure holds variable and constant names
// and values
//
typedef struct
   {
   PSZ     pszCon;
   BIG     big;
   } MVAL;


FNDESCR mathfn[]={{"sin",   (PMTHFN) sinl},
                  {"cos",   (PMTHFN) cosl},
                  {"tan",   (PMTHFN) tanl},
                  {"asin",  (PMTHFN) asinl},
                  {"acos",  (PMTHFN) acosl},
                  {"atan",  (PMTHFN) atanl},
                  {"abs",   (PMTHFN) fabsl},
                  {"ceil",  (PMTHFN) ceill},
                  {"cosh",  (PMTHFN) coshl},
                  {"sinh",  (PMTHFN) sinhl},
                  {"exp",   (PMTHFN) expl},
                  {"floor", (PMTHFN) floorl},
                  {"log10", (PMTHFN) log10l},
                  {"log",   (PMTHFN) logl},
                  {"sqrt",  (PMTHFN) sqrtl},
                  {"",      NULL}
                  };

MVAL mathconst[]={{"pi", 3.1415926535},
                  {"e",  2.302585},
                  {"",   0.0}
                  };

#define MAXVARS 20
MVAL mathval[MAXVARS] ={{NULL, 0.0}, {"@QUAN", 0.0}, 
                        {NULL, 0.0}, {NULL,    0.0},
                        {NULL, 0.0}, {NULL,    0.0},
                        {NULL, 0.0}, {NULL,    0.0},
                        {NULL, 0.0}, {NULL,    0.0},
                        {NULL, 0.0}, {NULL,    0.0},
                        {NULL, 0.0}, {NULL,    0.0},
                        {NULL, 0.0}, {NULL,    0.0},
                        {NULL, 0.0}, {NULL,    0.0},
                        {NULL, 0.0}, {NULL,    0.0}};
USHORT LASTVAR = 0;



char *MathErrors[] = {"No Error",                         // 0
                      "Closing Parenthesis expected ')'", // 1
                      "Opening Parenthesis expected '('", // 2
                      "Unrecognized character",           // 3
                      "Empty String",                     // 4
                      "",                                 // 5
                      NULL};                                  




// Reported Error Number
//
USHORT EVALERR     = 0;

// Reported Error Description
//
PSZ    EVALERRSTR  = NULL;

// Reported offending character position
//
USHORT EVALERRIDX  = 0;

// Internal, address of error position
//
PSZ    EVALERRPOS  = NULL;



/********************************************************************/
/*                                                                  */
/*  Error Routines                                                  */
/*                                                                  */
/********************************************************************/

BIG SetMathErr (USHORT uErr, PSZ pszErrPos)
   {
   BIG bg; 

//   InvalidateBIG (&bg);
   bg = 0.0;

   if (EVALERR)
      return bg;

   EVALERR    = uErr;
   EVALERRSTR = MathErrors [uErr];
   EVALERRPOS = pszErrPos;

   return bg;
   }




/********************************************************************/
/*                                                                  */
/*  BIG Math Routines                                               */
/*                                                                  */
/********************************************************************/


BOOL ValidBIG (BIG bgNum)
   {
   PULONG  pulTmp = (PULONG)(PVOID)&(bgNum);

   return !(*pulTmp == MARK);
   }



void InvalidateBIG (PBIG pbgNum)
   {
   PULONG  pulTmp = (PULONG)(PVOID)pbgNum;

   *pulTmp = MARK;
   }


int SkipWhite (PPSZ ppsz)
   {
   while (**ppsz && strchr (" \t", **ppsz)) 
      (*ppsz)++;
   return **ppsz;
   }


int MthEat (PPSZ ppsz, PSZ pszList)
   {
   SkipWhite (ppsz);
   if (!**ppsz || !strchr (pszList, **ppsz))
      return 0;
   return *(*ppsz)++;
   }


BOOL IsNumber (PSZ psz)
   {
   SkipWhite (&psz);
   if (*psz && strchr ("-+", *psz))
      {
      psz++;
      SkipWhite (&psz);
      }
   return !!(psz && strchr ("0123456789", *psz)) ;
   }


BIG  EatNumber (PPSZ ppszExp)
   {
   BIG big;

   big = _atold (*ppszExp);

   SkipWhite (ppszExp);
   if (**ppszExp && strchr ("-+",               **ppszExp)) 
      (*ppszExp)++;
   while (**ppszExp && strchr ("0123456789",    **ppszExp)) 
      (*ppszExp)++;
   if (**ppszExp && strchr (".",                **ppszExp)) 
      (*ppszExp)++;
   while (**ppszExp && strchr ("0123456789",    **ppszExp)) 
      (*ppszExp)++;
   if (**ppszExp && strchr ("dDeE",             **ppszExp))
      {
      (*ppszExp)++;
      if (**ppszExp && strchr ("-+",            **ppszExp)) 
         (*ppszExp)++;      
      while (**ppszExp && strchr ("0123456789", **ppszExp)) 
         (*ppszExp)++;
      }
   return big;
   }



/*********************************************************/
/*                 Variable Code                         */
/*********************************************************/



USHORT MakeVar (PPSZ ppszVar)
   {
   char szTmp [40];
   PSZ  pszTmp;
   USHORT i;

   for (i=1; i<MAXVARS && mathval[i].pszCon; i++)
      ;
   if (i == MAXVARS)  /*--- no more room ---*/
      return 0;

   pszTmp = szTmp;
   *pszTmp++= *(*ppszVar)++;

   while (isalnum (**ppszVar))
      *pszTmp++= *(*ppszVar)++;
   *pszTmp = '\0';
   mathval[i].pszCon = strdup (szTmp);
   mathval[i].big    = 0.0;
   return i;
   }


USHORT FindVar (PPSZ ppszVar)
   {
   USHORT i, uLen;

   for (i=1; i<MAXVARS; i++)
      {
      if (!mathval[i].pszCon)
         continue;
      uLen = strlen (mathval[i].pszCon);
      if (!strnicmp (mathval[i].pszCon, *ppszVar, uLen) && !isalnum((*ppszVar)[uLen]))
         {
         *ppszVar += uLen;
         return i;
         }
      }
   return MakeVar (ppszVar);
   }


BOOL IsVar (PPSZ ppsz)
   {
   SkipWhite (ppsz);
   return (**ppsz == '@');
   }


BIG GetVar (PPSZ ppszVar)
   {
   USHORT i;

   if (!(i = FindVar (ppszVar)))
      return 0;
   LASTVAR = i;
   return mathval[i].big;
   }



BIG SetVar (USHORT uIndex, BIG big)
   {
   if (!uIndex || uIndex >= MAXVARS)
      return big;
   return mathval[uIndex].big = big;
   }



/*
 * eats a parenthesized expression
 * open parens already eaten
 *
 */

BIG ParenExp (PPSZ ppszExp)
   {
   BIG    big;

   big = _Eval (ppszExp, 0);
   if (!MthEat (ppszExp, ")"))
      return SetMathErr (1, *ppszExp);
   return big;
   }



BIG EvalAtom (PPSZ ppszExp)
   {
   USHORT i, l;

   i = l =0;
   /*--- Number ---*/
   if (IsNumber (*ppszExp))
      return EatNumber (ppszExp);

   /*--- Parenthesized Expression ---*/
   if (MthEat (ppszExp, "("))
      return ParenExp (ppszExp);

   /*--- Function ---*/
   for (i=0; mathfn[i].pfn; i++)
      {
      if (strnicmp (mathfn[i].pszFn, *ppszExp, l=strlen (mathfn[i].pszFn)))
         continue;
      *ppszExp += l;
      if (MthEat (ppszExp, "("))
         return mathfn[i].pfn(ParenExp (ppszExp));
      else
         return SetMathErr (2, *ppszExp);
      }

   /*--- Constant ---*/
   for (i=0; *(mathconst[i].pszCon); i++)
      if (!strnicmp (mathconst[i].pszCon, *ppszExp, l=strlen (mathconst[i].pszCon)))
         {
         *ppszExp += l;
         return mathconst[i].big;
         }

   /*--- variables ---*/
   if (IsVar (ppszExp))
      return GetVar (ppszExp);

   return SetMathErr (3, *ppszExp);
   }


#define ATOMLEVEL 5
PSZ LevelOps[] = {"=", "<>", "+-", "*/%", "^"};

//   Use this to disallow assignments to variables
//
//   #define ATOMLEVEL 4
//   PSZ LevelOps[] = {"<>", "+-", "*/%", "^"};


BIG _Eval (PPSZ ppszExp, USHORT uLevel)
   {
   BIG big, bgTmp;
   int Op, j;

   j=0;
   if (uLevel == ATOMLEVEL)
      return EvalAtom (ppszExp);
   big = _Eval (ppszExp, uLevel+1);

   while (TRUE)
      switch (Op = MthEat (ppszExp, LevelOps[uLevel]))
         {
         case '=': j = LASTVAR; big = _Eval (ppszExp, uLevel +1); SetVar (j, big); break;
         case '<': big *= powl (10, _Eval (ppszExp, uLevel +1)); break;
         case '>': big /= powl (10, _Eval (ppszExp, uLevel +1)); break;
         case '+': big += _Eval (ppszExp, uLevel +1); break;
         case '-': big -= _Eval (ppszExp, uLevel +1); break;
         case '*': big *= _Eval (ppszExp, uLevel +1); break;
         case '/': bgTmp = _Eval (ppszExp, uLevel +1);
                   if (bgTmp < 1E-10 && bgTmp > -1E-10)
                      EVALERR = 3;
                   else
                      big /= bgTmp;
                   break;
         case '%': big  = fmodl(big, _Eval (ppszExp, uLevel +1)); break;
         case '^': big  = powl (big, _Eval (ppszExp, uLevel +1)); break;
         default : return big;
         }
   }


PBIG AToBIG (PBIG pbig, PSZ psz)
   {
   int c;
   PSZ p;

   LASTVAR = 0;
   EVALERR = 0;
   p = psz;

   if (!p || *p == '\0' || *p == '\x0A' || *p == '\x0D')
      SetMathErr (4, p);
   else
      {
      *pbig = _Eval (&p, 0);

      if ((c = SkipWhite (&p)) && c != '\x0D' && c!= '\x0A')
         SetMathErr (3, p);
      }
   EVALERRIDX = (EVALERR ? EVALERRPOS - psz: 0);

   if (EVALERR)
      InvalidateBIG (pbig);

   return pbig;
   }




/*********************************************************/
/*                 Test Fns                              */
/*********************************************************/


BOOL GetStr (PSZ pszStr)
   {
   int c;

   printf ("===> ");
   while ((c = getche()) != 0x0D)
      if (!c || c == 0xE0)
         getch ();
      else
         *pszStr++ = (char)c;
   *pszStr = '\0';
   return (c == 0x1B);
   }


main (int argc, char *argv[])
   {
   BIG  big;
   char sz[90];
   PSZ  p;
   USHORT i;

   while (TRUE)
      {
      GetStr (sz);
      p = sz;
      printf ("\n%s", sz);
      AToBIG (&big, p);

      if (EVALERR && EVALERR != 4)
         {
         printf ("\nError: %s\n", sz);
         for (i=EVALERRIDX + 7; i ; i--)
            putchar (' ');
         putchar ('^');
         printf ("%s\n",EVALERRSTR);
         }
      else
         printf (" = %lf\n", (double) big);
      }
   return 0;
   }


