/*
 * $Id: //websites/unixwiz/newroot/techtips/remap-pipe-fds.c.txt#1 $
 *
 * written by : Stephen J Friedl
 *              Software Consultant
 *              Tustin, California USA
 *              steve@unixwiz.net -*- http://www.unixwiz.net
 *
 *  Given two file descriptors that we obtained from the pipe(2) system
 *  call, map them to stdin(0) and stdout(1) of the current process.
 *  The usual approach is:
 *
 *    dup2(rpipe, 0); close(rpipe);
 *    dup2(wpipe, 1); close(wpipe);
 *
 *  and this works most of the time. But what happens if one of these
 *  descriptors is *already* 0 or 1 - then we end up making a mess of
 *  the descriptors.
 *
 *  The thing that makes this so hard to figure out is that when it's
 *  run from a "regular" user process, the pipe descriptors won't *ever*
 *  be in this low range, but the file-descriptor environment of a
 *  >daemon< is entirely different.
 *
 *  So: to account for this properly  we consider a kind of matrix that
 *  shows all the possible combinations of inputs and how we are to
 *  properly shuffle the FDs around. Each FD is either "0", "1", or
 *  ">1", the latter representing the usual case of being "out of the
 *  way" and not in any risk of getting stomped onn.
 *
 *  NOTE: in this table we use the same macro that we use in the C code
 *  below -- DUP2CLOSE(oldfd, newfd) -- that performs the obvious
 *  operations.
 *
 *       RFD  WFD     Required actions
 *       ---  ---   -------------------------
 *      [A]   0    1      *nothing*
 *
 *      [B]  >1   >1 \__/ DUP2CLOSE(rfd, 0); \___ same actions
 *      [C]   1   >1 /  \ DUP2CLOSE(wfd, 1); /    for both cases
 *        
 *
 *      [D]   0   >1      DUP2CLOSE(wfd, 1);
 *        
 *      [E]  >1    1      DUP2CLOSE(rfd, 0);
 *
 *      [F]  >1    0      DUP2CLOSE(wfd, 1);
 *                        DUP2CLOSE(rfd, 0);
 *
 *      [G]   1    0      tmp = dup(wfd); close(wfd);
 *                        DUP2CLOSE(rfd, 0);
 *                        DUP2CLOSE(tmp, 1);
 *
 *  Return is TRUE if all is well or FALSE on error.
 *
 * STUFF TO DO
 * -----------
 *
 *  * Extend this to handle stdin/stdout/STDERR
 *
 *  * write a version that works using only dup(), not dup2()
 */

#include <unistd.h>

#ifndef TRUE
#  define TRUE 1
#endif

#ifndef FALSE
#  define FALSE 0
#endif

/*------------------------------------------------------------------------
 * Every time we run a dup2(), we always close the old FD, so this macro
 * runs them both together and evaluates to TRUE if it all went OK and 
 * FALSE if not.
 */
#define DUP2CLOSE(oldfd, newfd) (dup2(oldfd, newfd) == 0  &&  close(oldfd) == 0)

int remap_pipe_stdin_stdout(int rpipe, int wpipe)
{
  /*------------------------------------------------------------------
   * CASE [A]
   *
   * This is the trivial case that probably never happens: the two FDs
   * are already in the right place and we have nothing to do. Though
   * this probably doesn't happen much, it's guaranteed that *doing*
   * any shufflingn would close descriptors that shouldn't have been.
   */
  if ( rpipe == 0  &&  wpipe == 1 )
    return TRUE;

  /*----------------------------------------------------------------
   * CASE [B] and [C]
   *
   * These two have the same handling but not the same rules. In case
   * [C] where both FDs are "out of the way", it doesn't matter which
   * of the FDs is closed first, but in case [B] it MUST be done in
   * this order.
   */
  if ( rpipe >= 1  &&  wpipe > 1 )
  {
    return DUP2CLOSE(rpipe, 0)
        && DUP2CLOSE(wpipe, 1);
  }


  /*----------------------------------------------------------------
   * CASE [D]
   * CASE [E]
   *
   * In these cases, *one* of the FDs is already correct and the other
   * one can just be dup'd to the right place:
   */
  if ( rpipe == 0  &&  wpipe >= 1 )
    return DUP2CLOSE(wpipe, 1);

  if ( rpipe  >= 1  &&  wpipe == 1 )
    return DUP2CLOSE(rpipe, 0);


  /*----------------------------------------------------------------
   * CASE [F]
   *
   * Here we have the write pipe in the read slot, but the read FD
   * is out of the way: this means we can do this in just two steps
   * but they MUST be in this order.
   */
  if ( rpipe >= 1   &&  wpipe == 0 )
  {
    return DUP2CLOSE(wpipe, 1)
        && DUP2CLOSE(rpipe, 0);
  }

  /*----------------------------------------------------------------
   * CASE [G]
   *
   * This is the trickiest case because the two file descriptors  are
   * *backwards*, and the only way to make it right is to make a
   * third temporary FD during the swap.
   */
  if ( rpipe == 1  &&  wpipe == 0 )
  {
  const int tmp = dup(wpipe);   /* NOTE! this is not dup2() ! */

    return  tmp > 1
        &&  close(wpipe)   == 0
        &&  DUP2CLOSE(rpipe, 0)
        &&  DUP2CLOSE(tmp,   1);
  }

  /* SHOULD NEVER GET HERE */

  return  FALSE;
}
