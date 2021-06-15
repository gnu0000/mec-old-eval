/*
 * expression evaluator
 *
 * Craig Fitzgerald
 *
 *
 */

#include <os2.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>




typedef long   double BIG;
typedef PSZ    *PPSZ;
typedef BIG    (*PMTHFN) (BIG big);

BIG _Eval (PPSZ ppszExp, USHORT uLevel);

typedef struct
   {
   PSZ     pszFn;
   PMTHFN  pfn;
   } FNDESCR;

typedef struct
   {
   PSZ     pszCon;
   BIG     big;
   } MVAL;

FNDESCR mathfn[] = {{"sin",   (PMTHFN) sinl},
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
                    {"log",   (PMTHFN) logl},
                    {"log10", (PMTHFN) log10l},
                    {"sqrt",  (PMTHFN) sqrtl},
                    {"",      NULL}
                   };

MVAL mathconst[]=  {{"pi", 3.1415926535},
                    {"e",  2.302585},
                    {"",   0.0}
                   };


#define MAXVARS 20
USHORT LASTVAR = 0;

MVAL mathval[MAXVARS] = {{NULL, 0.0}, {NULL, 0.0},
                         {NULL, 0.0}, {NULL, 0.0},
                         {NULL, 0.0}, {NULL, 0.0},
                         {NULL, 0.0}, {NULL, 0.0},
                         {NULL, 0.0}, {NULL, 0.0},
                         {NULL, 0.0}, {NULL, 0.0},
                         {NULL, 0.0}, {NULL, 0.0},
                         {NULL, 0.0}, {NULL, 0.0},
                         {NULL, 0.0}, {NULL, 0.0},
                         {NULL, 0.0}, {NULL, 0.0}};
              



int SkipWhite (PPSZ ppsz)
   {
   while (**ppsz && strchr (" \t", **ppsz)) 
      (*ppsz)++;
   return **ppsz;
   }



int Eat (PPSZ ppsz, PSZ pszList)
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



#define ATOMLEVEL 6

PSZ LevelOps (USHORT uLevel)
   {
   switch (uLevel)
      {
      case 1: return "=";
      case 2: return "<>";
      case 3: return "+-";
      case 4: return "*/%";
      case 5: return "^";
      }
   }


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
      if (!strncmp (mathval[i].pszCon, *ppszVar, uLen) && !isalnum((*ppszVar)[uLen]))
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
   if (uIndex <1 || uIndex >= MAXVARS)
      return big;
   return mathval[uIndex].big = big;
   }


BIG EvalAtom (PPSZ ppszExp)
   {
   USHORT i, l;
   BIG    big;

   /*--- Number ---*/
   if (IsNumber (*ppszExp))
      return EatNumber (ppszExp);

   /*--- Parenthesized Expression ---*/
   if (Eat (ppszExp, "("))
      {
      big = _Eval (ppszExp, 1);
      if (!Eat (ppszExp, ")"))
         printf ("Expected ) got %c", **ppszExp);
      return big;
      }

   /*--- Function ---*/
   for (i=0; mathfn[i].pfn; i++)
      if (!strnicmp (mathfn[i].pszFn, *ppszExp, l=strlen (mathfn[i].pszFn)))
         {
         *ppszExp += l;
         return mathfn[i].pfn(_Eval (ppszExp, 1));
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

   printf ("Expected digit or (, got %c", **ppszExp);
   exit (1);
   }




BIG _Eval (PPSZ ppszExp, USHORT uLevel)
   {
   BIG big;
   int Op, j;

   if (uLevel == ATOMLEVEL)
      return EvalAtom (ppszExp);

   big = _Eval (ppszExp, uLevel+1);

   while (TRUE)
      switch (Op = Eat (ppszExp, LevelOps (uLevel)))
         {
         case '=': j = LASTVAR; big = _Eval (ppszExp, uLevel +1); SetVar (j, big); break;
         case '<': big *= powl (10, _Eval (ppszExp, uLevel +1)); break;
         case '>': big /= powl (10, _Eval (ppszExp, uLevel +1)); break;
         case '+': big += _Eval (ppszExp, uLevel +1); break;
         case '-': big -= _Eval (ppszExp, uLevel +1); break;
         case '*': big *= _Eval (ppszExp, uLevel +1); break;
         case '/': big /= _Eval (ppszExp, uLevel +1); break;
         case '%': big  = fmodl(big, _Eval (ppszExp, uLevel +1)); break;
         case '^': big  = powl (big, _Eval (ppszExp, uLevel +1)); break;
         default : return big;
         }
   }


BIG EvalExp (PPSZ ppszExpr)
   {
   BIG big;

   LASTVAR = 0;
   big = _Eval (ppszExpr, 1);

   if (SkipWhite (ppszExpr))
      {
      printf ("Illegal Expresson construct : '%s'", *ppszExpr);
      return 0.0;
      }
   return big;
   }



BOOL GetStr (PSZ pszStr)
   {
   int c;

   printf ("Enter Expression => ");
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

   while (TRUE)
      {
      GetStr (sz);
      p = sz;
      printf ("\n%s = ", sz);
      big = EvalExp (&p);
      printf ("%lf\n", (double) big);
      }

   return 0;
   }

