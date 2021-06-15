/*
 *
 * eval2.c
 * Wednesday, 4/19/1995.
 *
 */

#define INCL_VIO
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <GnuMath.h>
#include <GnuKbd.h>
#include <GnuScr.h>

int main (int argc, char *argv[])
   {
   USHORT x, y, uErrIdx;
   char   szIn[256], szNum[256];
   BIG    bgVal;
   PSZ    pszErr;

   ScrInitMetrics ();
   while (TRUE)
      {
      GnuGetCursor (NULL, &y, &x);
      printf (":");
      KeyEditCell (szIn, y, 2, 75, 0);
      GnuMoveCursor (NULL, y, 0);
      printf ("\n");
      GnuGetCursor (NULL, &y, &x);

      AToBIG (&bgVal, szIn);
      if (MthIsError (&pszErr, &uErrIdx))
         {
         GnuPaint (NULL, y, uErrIdx+3, 1, 1, "^");
         GnuMoveCursor (NULL, y, 0);
         printf ("\n%s\n", pszErr);
         }
      else
         {
         MthFmat (szNum, bgVal, 40, 10, TRUE, FALSE);
         printf ("= %s\n", szNum);
         }
      }
   return 0;
   }

