/*******************************************************************************
 *
 * file: helpers.c
 *
 * FreeETarget helper files
 *
 *******************************************************************************
 *
 * Helper files that have no pre-defined home
 *
 ******************************************************************************/
#include <stdio.h>
#include "string.h"
#include "ctype.h"

#include "freETarget.h"
#include "json.h"
#include "token.h"

/*-----------------------------------------------------
 *
 * @function: target_name
 *
 * @brief: Determine the target name and return
 *
 * @return: Target name returned via pointer
 *
 *-----------------------------------------------------
 *
 * Depending on the settings, determine the target name
 * and return it to the caller
 *
 *-----------------------------------------------------*/
void target_name(char *name_space)
{

  if ( (json_token == TOKEN_NONE) || (my_ring == TOKEN_UNDEF) )
  {
    if ( json_name_id != JSON_NAME_TEXT )
    {
      sprintf(name_space, "FET-%s", names[json_name_id]);
    }
    else
    {
      if ( json_name_text[0] != 0 )
      {
        sprintf(name_space, "FET-%s", json_name_text);
      }
      else
      {
        sprintf(name_space, "Undefined name");
      }
    }
  }
  else
  {
    sprintf(name_space, "FET-%d", my_ring);
  }

  /*
   * All done, return
   */
  return;
}

/*-----------------------------------------------------
 *
 * @function: to_int
 *
 * @brief: Convert an ASCII hex number to an integer
 *
 * @return: Integer value
 *
 *-----------------------------------------------------
 *
 * Depending on the settings, determine the target name
 * and return it to the caller
 *
 *-----------------------------------------------------*/
int to_int(char h)
{
  h = toupper(h);

  if ( h > '9' )
  {
    return 10 + (h - 'A');
  }
  else
  {
    return h - '0';
  }
}

/*-----------------------------------------------------
 *
 * @function: instr
 *
 * @brief: Compare two strings
 *
 * @return: Number of matching characters
 *
 *-----------------------------------------------------
 *
 * Compare two strings.
 * Return -1 if not equal,
 * length of string if equal
 * S1 Long String, S2 Short String .
 * instr("CAT Sam", "CAT") = 3
 * instr("CAT Sam", "CUT") == -1)
 *-----------------------------------------------------*/

int instr(char *s1, // Source string
          char *s2  // Comparison string
)
{
  int i;

  i = 0;
  while ( (*s1 != 0) && (*s2 != 0) )
  {
    if ( *s1 != *s2 )
    {
      return -1;
    }
    s1++;
    s2++;
    i++;
  }

  /*
   * Reached the end of the comparison string. Check that we arrived at a NULL
   */
  if ( *s2 == 0 ) // Both strings are the same
  {
    return i;
  }

  return -1;      // The strings are different
}
