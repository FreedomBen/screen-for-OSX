/*
 * rcs.h
 *
 * jw 9.2.92
 **************************************************
 * RCS_ID("$Id$ FAU");
 */

#ifndef __RCS_H__
# define __RCS_H__

# if !defined(lint)
#  ifdef __GNUC__
#   define RCS_ID(id) static char *rcs_id() { return rcs_id(id); }
#  else
#   define RCS_ID(id) static char *rcs_id = id;
#  endif /* !__GNUC__ */
# else
#  define RCS_ID(id)      /* Nothing */
# endif /* !lint */

#endif /* __RCS_H__ */
