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
//#include <ctype.h>




typedef long   double BIG;
typedef PSZ    *PPSZ;
typedef BIG    (*PMTHFN) (BIG big);

BIG _Eval (PPSZ ppszExp, USHORT uLevel);


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


#define ATOMLEVEL 5

PSZ LevelOps (USHORT uLevel)
   {
   switch (uLevel)
      {
      case 1: return "<>";
      case 2: return "+-";
      case 3: return "*/%";
      case 4: return "^";
      }
   }


BIG EvalAtom (PPSZ ppszExp)
   {
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

   printf ("Expected digit or (, got %c", **ppszExp);
   exit (1);
   }




BIG _Eval (PPSZ ppszExp, USHORT uLevel)
   {
   BIG big;
   int Op;

   if (uLevel == ATOMLEVEL)
      return EvalAtom (ppszExp);

   big = _Eval (ppszExp, uLevel+1);

   while (TRUE)
      switch (Op = Eat (ppszExp, LevelOps (uLevel)))
         {
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

