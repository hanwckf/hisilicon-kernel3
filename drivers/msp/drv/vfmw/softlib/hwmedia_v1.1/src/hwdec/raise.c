/*$$$!!Warning: Huawei key information asset. No spread without permission.$$$*/
/*CODEMARK:EG4uRhTwMmgcVFBsBnYHCDadN5jJKSuVyxmmaCmKFU6eJEbB2fyHF9weu4/jer/hxLHb+S1e
E0zVg4C3NiZh4SXu1DUPt3BK64ZJx2SaroQgYAryPbE4w1z7tx39CnajerL14+es+s0FHpIB
vtZ3TTHJIv8JT6mJobslEiaqR2+y2W51jdCGZgSjnlcVw/7IiAKeDTCbWqJmQAXR+StyUkMd
/czLeTGg94MFZr2cQxCxxhRnUQflabcVj++qnrtnsi+UdoSMqKwMrqMuAO3Qiw==#*/
/*$$$!!Warning: Deleting or modifying the preceding information is prohibited.$$$*/

/* Copyright (C) 1991,95,96,2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

//#include <signal.h>
//#include <errno.h>

/* Raise the signal SIG.  */
int
raise (sig)
int sig;
{
    //  __set_errno (ENOSYS);
    //  return -1;
    return 0;
}
//weak_alias (raise, gsignal)

//stub_warning (raise)
//stub_warning (gsignal)
//#include <stub-tag.h>
