/*	$NetBSD: localtime.c,v 1.72 2012/10/28 19:02:29 christos Exp $	*/

/* Don't reformat the code arbitrarily.

   It uses in wide parts the exact formatting as the upstream NetBSD
   versions.  The purpose is to simplify subsequent diffs to the NetBSD
   version, should the need arise again at one point. */

/*
** This file is in the public domain, so clarified as of
** 1996-06-05 by Arthur David Olson.
*/
/* Temporarily merged private.h and tzfile.h for ease of management - DJ */

#include "winsup.h"
#include "cygerrno.h"
#include "sync.h"
#include <ctype.h>
#define STD_INSPIRED
#define lint

#define USG_COMPAT

#ifndef lint
#ifndef NOID
static char	elsieid[] = "@(#)localtime.c	8.17";
#endif /* !defined NOID */
#endif /* !defined lint */

/*
** Leap second handling from Bradley White.
** POSIX-style TZ environment variable handling from Guy Harris.
*/

#define NO_ERROR_IN_DST_GAP

/*LINTLIBRARY*/

#ifndef PRIVATE_H

#define PRIVATE_H

/*
** This file is in the public domain, so clarified as of
** 1996-06-05 by Arthur David Olson
*/

/*
** This header is for use ONLY with the time conversion code.
** There is no guarantee that it will remain unchanged,
** or that it will remain at all.
** Do NOT copy it to any system include directory.
** Thank you!
*/

/*
** ID
*/

#ifndef lint
#ifndef NOID
static char	privatehid[] = "@(#)private.h	7.48";
#endif /* !defined NOID */
#endif /* !defined lint */

/*
** Nested includes
*/

#include "stdio.h"
#include "limits.h"	/* for CHAR_BIT */
#include "stdlib.h"
#include "unistd.h"	/* for F_OK and R_OK */

/* Unlike <ctype.h>'s isdigit, this also works if c < 0 | c > UCHAR_MAX.  */
#define is_digit(c) ((unsigned)(c) - '0' <= 9)

#ifndef __pure
#if 2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)
# define __pure __attribute__ ((__pure__))
#else
# define __pure /* empty */
#endif
#endif

/*
** Finally, some convenience items.
*/

#ifndef TYPE_INTEGRAL
#define TYPE_INTEGRAL(type) (/*CONSTCOND*/((type) 0.5) != 0.5)
#endif /* !defined TYPE_INTEGRAL */

#ifndef TYPE_BIT
#define TYPE_BIT(type)	(sizeof (type) * CHAR_BIT)
#endif /* !defined TYPE_BIT */

#ifndef TYPE_SIGNED
#define TYPE_SIGNED(type) (((type) -1) < 0)
#endif /* !defined TYPE_SIGNED */

#ifndef INT_STRLEN_MAXIMUM
/*
** 302 / 1000 is log10(2.0) rounded up.
** Subtract one for the sign bit if the type is signed;
** add one for integer division truncation;
** add one more for a minus sign if the type is signed.
*/
#define INT_STRLEN_MAXIMUM(type) \
    ((TYPE_BIT(type) - TYPE_SIGNED(type)) * 302 / 1000 + 1 + TYPE_SIGNED(type))
#endif /* !defined INT_STRLEN_MAXIMUM */

/*
** INITIALIZE(x)
*/

#ifndef GNUC_or_lint
#ifdef lint
#define GNUC_or_lint
#endif /* defined lint */
#ifndef lint
#ifdef __GNUC__
#define GNUC_or_lint
#endif /* defined __GNUC__ */
#endif /* !defined lint */
#endif /* !defined GNUC_or_lint */

#ifndef INITIALIZE
#ifdef GNUC_or_lint
#define INITIALIZE(x)	((x) = 0)
#endif /* defined GNUC_or_lint */
#ifndef GNUC_or_lint
#define INITIALIZE(x)
#endif /* !defined GNUC_or_lint */
#endif /* !defined INITIALIZE */

#ifndef TZ_DOMAIN
#define TZ_DOMAIN "tz"
#endif /* !defined TZ_DOMAIN */

#ifndef YEARSPERREPEAT
#define YEARSPERREPEAT	  400     /* years before a Gregorian repeat */
#endif /* !defined YEARSPERREPEAT */

/*
** The Gregorian year averages 365.2425 days, which is 31556952 seconds.
*/

#ifndef AVGSECSPERYEAR
#define AVGSECSPERYEAR	  31556952L
#endif /* !defined AVGSECSPERYEAR */

#ifndef SECSPERREPEAT
#define SECSPERREPEAT	   ((int_fast64_t) YEARSPERREPEAT * (int_fast64_t) AVGSECSPERYEAR)
#endif /* !defined SECSPERREPEAT */

#ifndef SECSPERREPEAT_BITS
#define SECSPERREPEAT_BITS      34      /* ceil(log2(SECSPERREPEAT)) */
#endif /* !defined SECSPERREPEAT_BITS */

/*
** UNIX was a registered trademark of UNIX System Laboratories in 1993.
*/

#endif /* !defined PRIVATE_H */

#ifndef TZFILE_H

#define TZFILE_H

/*
** This file is in the public domain, so clarified as of
** 1996-06-05 by Arthur David Olson.
*/

/*
** This header is for use ONLY with the time conversion code.
** There is no guarantee that it will remain unchanged,
** or that it will remain at all.
** Do NOT copy it to any system include directory.
** Thank you!
*/

/*
** ID
*/

#ifndef lint
#ifndef NOID
static char	tzfilehid[] = "@(#)tzfile.h	7.14";
#endif /* !defined NOID */
#endif /* !defined lint */

/*
** Information about time zone files.
*/

#ifndef TZDIR
#define TZDIR	"/usr/share/zoneinfo" /* Time zone object file directory */
#endif /* !defined TZDIR */

#ifndef TZDEFAULT
#define TZDEFAULT	"localtime"
#endif /* !defined TZDEFAULT */

#ifndef TZDEFRULES
#define TZDEFRULES	"posixrules"
#endif /* !defined TZDEFRULES */

/*
** Each file begins with. . .
*/

#define	TZ_MAGIC	"TZif"

struct tzhead {
	char	tzh_magic[4];		/* TZ_MAGIC */
	char	tzh_version[1];		/* '\0' or '2' as of 2005 */
	char	tzh_reserved[15];	/* reserved for future use */
	char	tzh_ttisgmtcnt[4];	/* coded number of trans. time flags */
	char	tzh_ttisstdcnt[4];	/* coded number of trans. time flags */
	char	tzh_leapcnt[4];		/* coded number of leap seconds */
	char	tzh_timecnt[4];		/* coded number of transition times */
	char	tzh_typecnt[4];		/* coded number of local time types */
	char	tzh_charcnt[4];		/* coded number of abbr. chars */
};

/*
** . . .followed by. . .
**
**	tzh_timecnt (char [4])s		coded transition times a la time(2)
**	tzh_timecnt (unsigned char)s	types of local time starting at above
**	tzh_typecnt repetitions of
**		one (char [4])		coded UTC offset in seconds
**		one (unsigned char)	used to set tm_isdst
**		one (unsigned char)	that's an abbreviation list index
**	tzh_charcnt (char)s		'\0'-terminated zone abbreviations
**	tzh_leapcnt repetitions of
**		one (char [4])		coded leap second transition times
**		one (char [4])		total correction after above
**	tzh_ttisstdcnt (char)s		indexed by type; if TRUE, transition
**					time is standard time, if FALSE,
**					transition time is wall clock time
**					if absent, transition times are
**					assumed to be wall clock time
**	tzh_ttisgmtcnt (char)s		indexed by type; if TRUE, transition
**					time is UTC, if FALSE,
**					transition time is local time
**					if absent, transition times are
**					assumed to be local time
*/

/*
** If tzh_version is '2' or greater, the above is followed by a second instance
** of tzhead and a second instance of the data in which each coded transition
** time uses 8 rather than 4 chars,
** then a POSIX-TZ-environment-variable-style string for use in handling
** instants after the last transition time stored in the file
** (with nothing between the newlines if there is no POSIX representation for
** such instants).
*/

/*
** In the current implementation, "tzset()" refuses to deal with files that
** exceed any of the limits below.
*/

#ifndef TZ_MAX_TIMES
/*
** The TZ_MAX_TIMES value below is enough to handle a bit more than a
** year's worth of solar time (corrected daily to the nearest second) or
** 138 years of Pacific Presidential Election time
** (where there are three time zone transitions every fourth year).
*/
#define TZ_MAX_TIMES	1200
#endif /* !defined TZ_MAX_TIMES */

#ifndef TZ_MAX_TYPES
#ifndef NOSOLAR
#define TZ_MAX_TYPES	256 /* Limited by what (unsigned char)'s can hold */
#endif /* !defined NOSOLAR */
#ifdef NOSOLAR
/*
** Must be at least 14 for Europe/Riga as of Jan 12 1995,
** as noted by Earl Chew.
*/
#define TZ_MAX_TYPES	20	/* Maximum number of local time types */
#endif /* !defined NOSOLAR */
#endif /* !defined TZ_MAX_TYPES */

#ifndef TZ_MAX_CHARS
#define TZ_MAX_CHARS	50	/* Maximum number of abbreviation characters */
				/* (limited by what unsigned chars can hold) */
#endif /* !defined TZ_MAX_CHARS */

#ifndef TZ_MAX_LEAPS
#define TZ_MAX_LEAPS	50	/* Maximum number of leap second corrections */
#endif /* !defined TZ_MAX_LEAPS */

#define SECSPERMIN	60
#define MINSPERHOUR	60
#define HOURSPERDAY	24
#define DAYSPERWEEK	7
#define DAYSPERNYEAR	365
#define DAYSPERLYEAR	366
#define SECSPERHOUR	(SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	((long) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR	12

#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

#define TM_JANUARY	0
#define TM_FEBRUARY	1
#define TM_MARCH	2
#define TM_APRIL	3
#define TM_MAY		4
#define TM_JUNE		5
#define TM_JULY		6
#define TM_AUGUST	7
#define TM_SEPTEMBER	8
#define TM_OCTOBER	9
#define TM_NOVEMBER	10
#define TM_DECEMBER	11

#define TM_YEAR_BASE	1900

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY

/*
** Accurate only for the past couple of centuries;
** that will probably do.
*/

#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))

#endif /* !defined TZFILE_H */

#include "fcntl.h"

#ifdef __TM_GMTOFF
# define TM_GMTOFF __TM_GMTOFF
#endif
#ifdef __TM_ZONE
# define TM_ZONE __TM_ZONE
#endif

/*
** SunOS 4.1.1 headers lack O_BINARY.
*/

#ifdef O_BINARY
#define OPEN_MODE	(O_RDONLY | O_BINARY)
#endif /* defined O_BINARY */
#ifndef O_BINARY
#define OPEN_MODE	O_RDONLY
#endif /* !defined O_BINARY */

#ifndef WILDABBR
/*
** Someone might make incorrect use of a time zone abbreviation:
**	1.	They might reference tzname[0] before calling tzset (explicitly
**		or implicitly).
**	2.	They might reference tzname[1] before calling tzset (explicitly
**		or implicitly).
**	3.	They might reference tzname[1] after setting to a time zone
**		in which Daylight Saving Time is never observed.
**	4.	They might reference tzname[0] after setting to a time zone
**		in which Standard Time is never observed.
**	5.	They might reference tm.TM_ZONE after calling offtime.
** What's best to do in the above cases is open to debate;
** for now, we just set things up so that in any of the five cases
** WILDABBR is used. Another possibility: initialize tzname[0] to the
** string "tzname[0] used before set", and similarly for the other cases.
** And another: initialize tzname[0] to "ERA", with an explanation in the
** manual page of what this "time zone abbreviation" means (doing this so
** that tzname[0] has the "normal" length of three characters).
*/
#define WILDABBR	"   "
#endif /* !defined WILDABBR */

static const char	wildabbr[] = WILDABBR;

static const char	gmt[] = "GMT";

/*
** The DST rules to use if TZ has no rules and we can't load TZDEFRULES.
** We default to US rules as of 1999-08-17.
** POSIX 1003.1 section 8.1.1 says that the default DST rules are
** implementation dependent; for historical reasons, US rules are a
** common default.
*/
#ifndef TZDEFRULESTRING
#define TZDEFRULESTRING ",M4.1.0,M10.5.0"
#endif /* !defined TZDEFDST */

struct ttinfo {				/* time type information */
	long		tt_gmtoff;	/* UTC offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	int		tt_abbrind;	/* abbreviation list index */
	int		tt_ttisstd;	/* TRUE if transition is std time */
	int		tt_ttisgmt;	/* TRUE if transition is UTC */
};

struct lsinfo {				/* leap second information */
	time_t		ls_trans;	/* transition time */
	long		ls_corr;	/* correction to apply */
};

#define BIGGEST(a, b)	(((a) > (b)) ? (a) : (b))

#ifdef TZNAME_MAX
#define MY_TZNAME_MAX	TZNAME_MAX
#endif /* defined TZNAME_MAX */
#ifndef TZNAME_MAX
#define MY_TZNAME_MAX	255
#endif /* !defined TZNAME_MAX */

struct __state {
	int		leapcnt;
	int		timecnt;
	int		typecnt;
	int		charcnt;
	int		goback;
	int		goahead;
	time_t		ats[TZ_MAX_TIMES];
	unsigned char	types[TZ_MAX_TIMES];
	struct ttinfo	ttis[TZ_MAX_TYPES];
	char		chars[/*CONSTCOND*/BIGGEST(BIGGEST(TZ_MAX_CHARS + 1,
				sizeof gmt), (2 * (MY_TZNAME_MAX + 1)))];
	struct lsinfo	lsis[TZ_MAX_LEAPS];
};

typedef struct __state *timezone_t;

struct rule {
	int		r_type;		/* type of rule--see below */
	int		r_day;		/* day number of rule */
	int		r_week;		/* week number of rule */
	int		r_mon;		/* month number of rule */
	long		r_time;		/* transition time of rule */
};

#define JULIAN_DAY		0	/* Jn - Julian day */
#define DAY_OF_YEAR		1	/* n - day of year */
#define MONTH_NTH_DAY_OF_WEEK	2	/* Mm.n.d - month, week, day of week */

typedef struct tm *(*subfun_t)(const timezone_t sp, const time_t *timep,
			       long offset, struct tm *tmp);

/*
** Prototypes for static functions.
*/

static long		detzcode(const char * codep);
static time_t		detzcode64(const char * codep);
static int		differ_by_repeat(time_t t1, time_t t0);
static const char *	getzname(const char * strp) __pure;
static const char *	getqzname(const char * strp, const int delim) __pure;
static const char *	getnum(const char * strp, int * nump, int min,
				int max);
static const char *	getsecs(const char * strp, long * secsp);
static const char *	getoffset(const char * strp, long * offsetp);
static const char *	getrule(const char * strp, struct rule * rulep);
static void		gmtload(timezone_t sp);
static struct tm *	gmtsub(const timezone_t sp, const time_t *timep,
				long offset, struct tm * tmp);
static struct tm *	localsub(const timezone_t sp, const time_t *timep,
				long offset, struct tm *tmp);
static int		increment_overflow(int * number, int delta);
static int		leaps_thru_end_of(int y) __pure;
static int		long_increment_overflow(long * number, int delta);
static int		long_normalize_overflow(long * tensptr,
				int * unitsptr, int base);
static int		normalize_overflow(int * tensptr, int * unitsptr,
				int base);
static void		settzname(void);
static time_t		time1(const timezone_t sp, struct tm * const tmp,
				subfun_t funcp, const long offset);
static time_t		time2(const timezone_t sp, struct tm * const tmp,
				subfun_t funcp,
				const long offset, int *const okayp);
static time_t		time2sub(const timezone_t sp, struct tm * const tmp,
				subfun_t funcp, const long offset,
				int *const okayp, const int do_norm_secs);
static struct tm *	timesub(const timezone_t sp, const time_t * timep,
				long offset, struct tm * tmp);
static int		tmcomp(const struct tm * atmp,
				const struct tm * btmp);
static time_t		transtime(time_t janfirst, int year,
				const struct rule * rulep, long offset) __pure;
static int		typesequiv(const timezone_t sp, int a, int b);
static int		tzload(timezone_t sp, const char * name,
				int doextend);
static int		tzparse(timezone_t sp, const char * name,
				int lastditch);
static void		tzset_unlocked(void);
static long		leapcorr(const timezone_t sp, time_t * timep);

static timezone_t lclptr;
static timezone_t gmtptr;

#ifndef TZ_STRLEN_MAX
#define TZ_STRLEN_MAX 255
#endif /* !defined TZ_STRLEN_MAX */

static char		lcl_TZname[TZ_STRLEN_MAX + 1];
static enum lcl_states
{
  lcl_setting = -1,
  lcl_unset = 0,
  lcl_from_environment = 1,
  lcl_from_default = 2
} lcl_is_set;
static int		gmt_is_set;

#define tzname _tzname
#undef _tzname

char *	tzname[2] = {
	(char *) wildabbr,
	(char *) wildabbr
};

/*
** Section 4.12.3 of X3.159-1989 requires that
**	Except for the strftime function, these functions [asctime,
**	ctime, gmtime, localtime] return values in one of two static
**	objects: a broken-down time structure and an array of char.
** Thanks to Paul Eggert for noting this.
*/

static struct tm	tm;

/* These variables are initialized by tzset.  The macro versions are
   defined in time.h, and indirect through the __imp_ pointers.  */

#define timezone _timezone
#define daylight _daylight
#undef _timezone
#undef _daylight

#ifdef USG_COMPAT
long			timezone = 0;
int			daylight;
#endif /* defined USG_COMPAT */

#ifdef ALTZONE
time_t			altzone = 0;
#endif /* defined ALTZONE */

static long
detzcode(const char *const codep)
{
	long	result;
	int	i;

	result = (codep[0] & 0x80) ? ~0L : 0;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return result;
}

static time_t
detzcode64(const char *const codep)
{
	time_t	result;
	int	i;

	result = (time_t)((codep[0] & 0x80) ? (~(int_fast64_t) 0) : 0);
	for (i = 0; i < 8; ++i)
		result = result * 256 + (codep[i] & 0xff);
	return result;
}

static void
settzname (void)
{
	timezone_t const	sp = lclptr;
	int			i;

	tzname[0] = (char *) wildabbr;
	tzname[1] = (char *) wildabbr;
#ifdef USG_COMPAT
	daylight = 0;
	timezone = 0;
#endif /* defined USG_COMPAT */
#ifdef ALTZONE
	altzone = 0;
#endif /* defined ALTZONE */
	if (sp == NULL) {
		tzname[0] = tzname[1] = (char *) gmt;
		return;
	}
	for (i = 0; i < sp->typecnt; ++i) {
		const struct ttinfo * const	ttisp = &sp->ttis[i];

		tzname[ttisp->tt_isdst] =
			&sp->chars[ttisp->tt_abbrind];
#ifdef USG_COMPAT
		if (ttisp->tt_isdst)
			daylight = 1;
		if (!ttisp->tt_isdst)
			timezone = -(ttisp->tt_gmtoff);
#endif /* defined USG_COMPAT */
#ifdef ALTZONE
		if (ttisp->tt_isdst)
			altzone = -(ttisp->tt_gmtoff);
#endif /* defined ALTZONE */
	}
	/*
	** And to get the latest zone names into tzname. . .
	*/
	for (i = 0; i < sp->timecnt; ++i) {
		const struct ttinfo *const ttisp = &sp->ttis[sp->types[i]];

		tzname[ttisp->tt_isdst] =
			&sp->chars[ttisp->tt_abbrind];
	}
}

#include "tz_posixrules.h"

static int
differ_by_repeat(const time_t t1, const time_t t0)
{
	if (TYPE_INTEGRAL(time_t) &&
		TYPE_BIT(time_t) - TYPE_SIGNED(time_t) < SECSPERREPEAT_BITS)
			return 0;
	return (int_fast64_t)t1 - (int_fast64_t)t0 == SECSPERREPEAT;
}

static int
tzload(timezone_t sp, const char *name, const int doextend)
{
	const char *		p;
	int			i;
	int			fid;
	int			stored;
	ssize_t			nread;
	typedef union {
		struct tzhead	tzhead;
		char		buf[2 * sizeof(struct tzhead) +
					2 * sizeof *sp +
					4 * TZ_MAX_TIMES];
	} u_t;
	u_t *			up;
	save_errno		save;

	up = (u_t *) calloc(1, sizeof *up);
	if (up == NULL)
		return -1;

	sp->goback = sp->goahead = FALSE;
	if (name == NULL && (name = TZDEFAULT) == NULL)
		goto oops;
	{
		int	doaccess;
		/*
		** Section 4.9.1 of the C standard says that
		** "FILENAME_MAX expands to an integral constant expression
		** that is the size needed for an array of char large enough
		** to hold the longest file name string that the implementation
		** guarantees can be opened."
		*/
		char		fullname[FILENAME_MAX + 1];

		if (name[0] == ':')
			++name;
		doaccess = name[0] == '/';
		if (!doaccess) {
			if ((p = TZDIR) == NULL)
				goto oops;
			if ((strlen(p) + strlen(name) + 1) >= sizeof fullname)
				goto oops;
			(void) strcpy(fullname, p);	/* XXX strcpy is safe */
			(void) strcat(fullname, "/");	/* XXX strcat is safe */
			(void) strcat(fullname, name);	/* XXX strcat is safe */
			/*
			** Set doaccess if '.' (as in "../") shows up in name.
			*/
			if (strchr(name, '.') != NULL)
				doaccess = TRUE;
			name = fullname;
		}
		if ((doaccess && access(name, R_OK) != 0)
		    || (fid = open(name, OPEN_MODE)) == -1)
		  {
		    const char *base = strrchr(name, '/');
		    if (base)
		      base++;
		    else
		      base = name;
		    if (strcmp(base, "posixrules"))
		      goto oops;

		    /* We've got a built-in copy of posixrules just in case */
		    fid = -2;
		  }
	}
	if (fid == -2)
	  {
	    memcpy(up->buf, _posixrules_data, sizeof (_posixrules_data));
	    nread = sizeof (_posixrules_data);
	  }
	else
	  {
	    nread = read(fid, up->buf, sizeof up->buf);
	    if (close(fid) < 0 || nread <= 0)
	      goto oops;
	  }
	for (stored = 4; stored <= 8; stored *= 2) {
		int		ttisstdcnt;
		int		ttisgmtcnt;

		ttisstdcnt = (int) detzcode(up->tzhead.tzh_ttisstdcnt);
		ttisgmtcnt = (int) detzcode(up->tzhead.tzh_ttisgmtcnt);
		sp->leapcnt = (int) detzcode(up->tzhead.tzh_leapcnt);
		sp->timecnt = (int) detzcode(up->tzhead.tzh_timecnt);
		sp->typecnt = (int) detzcode(up->tzhead.tzh_typecnt);
		sp->charcnt = (int) detzcode(up->tzhead.tzh_charcnt);
		p = up->tzhead.tzh_charcnt + sizeof up->tzhead.tzh_charcnt;
		if (sp->leapcnt < 0 || sp->leapcnt > TZ_MAX_LEAPS ||
			sp->typecnt <= 0 || sp->typecnt > TZ_MAX_TYPES ||
			sp->timecnt < 0 || sp->timecnt > TZ_MAX_TIMES ||
			sp->charcnt < 0 || sp->charcnt > TZ_MAX_CHARS ||
			(ttisstdcnt != sp->typecnt && ttisstdcnt != 0) ||
			(ttisgmtcnt != sp->typecnt && ttisgmtcnt != 0))
				goto oops;
		if (nread - (p - up->buf) <
			sp->timecnt * stored +		/* ats */
			sp->timecnt +			/* types */
			sp->typecnt * 6 +		/* ttinfos */
			sp->charcnt +			/* chars */
			sp->leapcnt * (stored + 4) +	/* lsinfos */
			ttisstdcnt +			/* ttisstds */
			ttisgmtcnt)			/* ttisgmts */
				goto oops;
		for (i = 0; i < sp->timecnt; ++i) {
			sp->ats[i] = (time_t)((stored == 4) ?
				detzcode(p) : detzcode64(p));
			p += stored;
		}
		for (i = 0; i < sp->timecnt; ++i) {
			sp->types[i] = (unsigned char) *p++;
			if (sp->types[i] >= sp->typecnt)
				goto oops;
		}
		for (i = 0; i < sp->typecnt; ++i) {
			struct ttinfo *	ttisp;

			ttisp = &sp->ttis[i];
			ttisp->tt_gmtoff = detzcode(p);
			p += 4;
			ttisp->tt_isdst = (unsigned char) *p++;
			if (ttisp->tt_isdst != 0 && ttisp->tt_isdst != 1)
				goto oops;
			ttisp->tt_abbrind = (unsigned char) *p++;
			if (ttisp->tt_abbrind < 0 ||
				ttisp->tt_abbrind > sp->charcnt)
					goto oops;
		}
		for (i = 0; i < sp->charcnt; ++i)
			sp->chars[i] = *p++;
		sp->chars[i] = '\0';	/* ensure '\0' at end */
		for (i = 0; i < sp->leapcnt; ++i) {
			struct lsinfo *	lsisp;

			lsisp = &sp->lsis[i];
			lsisp->ls_trans = (time_t)((stored == 4) ?
			    detzcode(p) : detzcode64(p));
			p += stored;
			lsisp->ls_corr = detzcode(p);
			p += 4;
		}
		for (i = 0; i < sp->typecnt; ++i) {
			struct ttinfo *	ttisp;

			ttisp = &sp->ttis[i];
			if (ttisstdcnt == 0)
				ttisp->tt_ttisstd = FALSE;
			else {
				ttisp->tt_ttisstd = *p++;
				if (ttisp->tt_ttisstd != TRUE &&
					ttisp->tt_ttisstd != FALSE)
						goto oops;
			}
		}
		for (i = 0; i < sp->typecnt; ++i) {
			struct ttinfo *	ttisp;

			ttisp = &sp->ttis[i];
			if (ttisgmtcnt == 0)
				ttisp->tt_ttisgmt = FALSE;
			else {
				ttisp->tt_ttisgmt = *p++;
				if (ttisp->tt_ttisgmt != TRUE &&
					ttisp->tt_ttisgmt != FALSE)
						goto oops;
			}
		}
		/*
		** Out-of-sort ats should mean we're running on a
		** signed time_t system but using a data file with
		** unsigned values (or vice versa).
		*/
		for (i = 0; i < sp->timecnt - 2; ++i)
			if (sp->ats[i] > sp->ats[i + 1]) {
				++i;
				if (TYPE_SIGNED(time_t)) {
					/*
					** Ignore the end (easy).
					*/
					sp->timecnt = i;
				} else {
					/*
					** Ignore the beginning (harder).
					*/
					int	j;

					for (j = 0; j + i < sp->timecnt; ++j) {
						sp->ats[j] = sp->ats[j + i];
						sp->types[j] = sp->types[j + i];
					}
					sp->timecnt = j;
				}
				break;
			}
		/*
		** If this is an old file, we're done.
		*/
		if (up->tzhead.tzh_version[0] == '\0')
			break;
		nread -= p - up->buf;
		for (i = 0; i < nread; ++i)
			up->buf[i] = p[i];
		/*
		** If this is a narrow integer time_t system, we're done.
		*/
		if (stored >= (int) sizeof(time_t)
/* CONSTCOND */
				&& TYPE_INTEGRAL(time_t))
			break;
	}
	if (doextend && nread > 2 &&
		up->buf[0] == '\n' && up->buf[nread - 1] == '\n' &&
		sp->typecnt + 2 <= TZ_MAX_TYPES) {
			struct __state ts;
			int	result;

			up->buf[nread - 1] = '\0';
			result = tzparse(&ts, &up->buf[1], FALSE);
			if (result == 0 && ts.typecnt == 2 &&
				sp->charcnt + ts.charcnt <= TZ_MAX_CHARS) {
					for (i = 0; i < 2; ++i)
						ts.ttis[i].tt_abbrind +=
							sp->charcnt;
					for (i = 0; i < ts.charcnt; ++i)
						sp->chars[sp->charcnt++] =
							ts.chars[i];
					i = 0;
					while (i < ts.timecnt &&
						ts.ats[i] <=
						sp->ats[sp->timecnt - 1])
							++i;
					while (i < ts.timecnt &&
					    sp->timecnt < TZ_MAX_TIMES) {
						sp->ats[sp->timecnt] =
							ts.ats[i];
						sp->types[sp->timecnt] =
							sp->typecnt +
							ts.types[i];
						++sp->timecnt;
						++i;
					}
					sp->ttis[sp->typecnt++] = ts.ttis[0];
					sp->ttis[sp->typecnt++] = ts.ttis[1];
			}
	}
	if (sp->timecnt > 1) {
		for (i = 1; i < sp->timecnt; ++i)
			if (typesequiv(sp, sp->types[i], sp->types[0]) &&
				differ_by_repeat(sp->ats[i], sp->ats[0])) {
					sp->goback = TRUE;
					break;
				}
		for (i = sp->timecnt - 2; i >= 0; --i)
			if (typesequiv(sp, sp->types[sp->timecnt - 1],
				sp->types[i]) &&
				differ_by_repeat(sp->ats[sp->timecnt - 1],
				sp->ats[i])) {
					sp->goahead = TRUE;
					break;
		}
	}
	free(up);
	/*
	** Get latest zone offsets into tzinfo (for newlib). . .
	*/
	if (sp == lclptr)
	  {
	    for (i = 0; i < sp->timecnt; ++i)
	      {
		const struct ttinfo *const ttisp = &sp->ttis[sp->types[i]];

		__gettzinfo ()->__tzrule[ttisp->tt_isdst].offset
				    = -ttisp->tt_gmtoff;
	      }
	  }
	return 0;
oops:
	free(up);
	return -1;
}

static int
typesequiv(const timezone_t sp, const int a, const int b)
{
	int	result;

	if (sp == NULL ||
		a < 0 || a >= sp->typecnt ||
		b < 0 || b >= sp->typecnt)
			result = FALSE;
	else {
		const struct ttinfo *	ap = &sp->ttis[a];
		const struct ttinfo *	bp = &sp->ttis[b];
		result = ap->tt_gmtoff == bp->tt_gmtoff &&
			ap->tt_isdst == bp->tt_isdst &&
			ap->tt_ttisstd == bp->tt_ttisstd &&
			ap->tt_ttisgmt == bp->tt_ttisgmt &&
			strcmp(&sp->chars[ap->tt_abbrind],
			&sp->chars[bp->tt_abbrind]) == 0;
	}
	return result;
}

static const int	mon_lengths[2][MONSPERYEAR] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static const int	year_lengths[2] = {
	DAYSPERNYEAR, DAYSPERLYEAR
};

/*
** Given a pointer into a time zone string, scan until a character that is not
** a valid character in a zone name is found. Return a pointer to that
** character.
*/

static const char *
getzname(const char *strp)
{
	char	c;

	while ((c = *strp) != '\0' && !is_digit(c) && c != ',' && c != '-' &&
		c != '+')
			++strp;
	return strp;
}

/*
** Given a pointer into an extended time zone string, scan until the ending
** delimiter of the zone name is located. Return a pointer to the delimiter.
**
** As with getzname above, the legal character set is actually quite
** restricted, with other characters producing undefined results.
** We don't do any checking here; checking is done later in common-case code.
*/

static const char *
getqzname(const char *strp, const int delim)
{
	int	c;

	while ((c = *strp) != '\0' && c != delim)
		++strp;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a number from that string.
** Check that the number is within a specified range; if it is not, return
** NULL.
** Otherwise, return a pointer to the first character not part of the number.
*/

static const char *
getnum(const char *strp, int *const nump, const int min, const int max)
{
	char	c;
	int	num;

	if (strp == NULL || !is_digit(c = *strp)) {
		errno = EINVAL;
		return NULL;
	}
	num = 0;
	do {
		num = num * 10 + (c - '0');
		if (num > max) {
			errno = EOVERFLOW;
			return NULL;	/* illegal value */
		}
		c = *++strp;
	} while (is_digit(c));
	if (num < min) {
		errno = EINVAL;
		return NULL;		/* illegal value */
	}
	*nump = num;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a number of seconds,
** in hh[:mm[:ss]] form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the number
** of seconds.
*/

static const char *
getsecs(const char *strp, long *const secsp)
{
	int	num;

	/*
	** `HOURSPERDAY * DAYSPERWEEK - 1' allows quasi-Posix rules like
	** "M10.4.6/26", which does not conform to Posix,
	** but which specifies the equivalent of
	** ``02:00 on the first Sunday on or after 23 Oct''.
	*/
	strp = getnum(strp, &num, 0, HOURSPERDAY * DAYSPERWEEK - 1);
	if (strp == NULL)
		return NULL;
	*secsp = num * (long) SECSPERHOUR;
	if (*strp == ':') {
		++strp;
		strp = getnum(strp, &num, 0, MINSPERHOUR - 1);
		if (strp == NULL)
			return NULL;
		*secsp += num * SECSPERMIN;
		if (*strp == ':') {
			++strp;
			/* `SECSPERMIN' allows for leap seconds. */
			strp = getnum(strp, &num, 0, SECSPERMIN);
			if (strp == NULL)
				return NULL;
			*secsp += num;
		}
	}
	return strp;
}

/*
** Given a pointer into a time zone string, extract an offset, in
** [+-]hh[:mm[:ss]] form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the time.
*/

static const char *
getoffset(const char *strp, long *const offsetp)
{
	int	neg = 0;

	if (*strp == '-') {
		neg = 1;
		++strp;
	} else if (*strp == '+')
		++strp;
	strp = getsecs(strp, offsetp);
	if (strp == NULL)
		return NULL;		/* illegal time */
	if (neg)
		*offsetp = -*offsetp;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a rule in the form
** date[/time]. See POSIX section 8 for the format of "date" and "time".
** If a valid rule is not found, return NULL.
** Otherwise, return a pointer to the first character not part of the rule.
*/

static const char *
getrule(const char *strp, struct rule *const rulep)
{
	if (*strp == 'J') {
		/*
		** Julian day.
		*/
		rulep->r_type = JULIAN_DAY;
		++strp;
		strp = getnum(strp, &rulep->r_day, 1, DAYSPERNYEAR);
	} else if (*strp == 'M') {
		/*
		** Month, week, day.
		*/
		rulep->r_type = MONTH_NTH_DAY_OF_WEEK;
		++strp;
		strp = getnum(strp, &rulep->r_mon, 1, MONSPERYEAR);
		if (strp == NULL)
			return NULL;
		if (*strp++ != '.')
			return NULL;
		strp = getnum(strp, &rulep->r_week, 1, 5);
		if (strp == NULL)
			return NULL;
		if (*strp++ != '.')
			return NULL;
		strp = getnum(strp, &rulep->r_day, 0, DAYSPERWEEK - 1);
	} else if (is_digit(*strp)) {
		/*
		** Day of year.
		*/
		rulep->r_type = DAY_OF_YEAR;
		strp = getnum(strp, &rulep->r_day, 0, DAYSPERLYEAR - 1);
	} else	return NULL;		/* invalid format */
	if (strp == NULL)
		return NULL;
	if (*strp == '/') {
		/*
		** Time specified.
		*/
		++strp;
		strp = getsecs(strp, &rulep->r_time);
	} else	rulep->r_time = 2 * SECSPERHOUR;	/* default = 2:00:00 */
	return strp;
}

/*
** Given the Epoch-relative time of January 1, 00:00:00 UTC, in a year, the
** year, a rule, and the offset from UTC at the time that rule takes effect,
** calculate the Epoch-relative time that rule takes effect.
*/

static time_t
transtime(const time_t janfirst, const int year, const struct rule *const rulep,
    const long offset)
{
	int	leapyear;
	time_t	value;
	int	i;
	int		d, m1, yy0, yy1, yy2, dow;

	INITIALIZE(value);
	leapyear = isleap(year);
	switch (rulep->r_type) {

	case JULIAN_DAY:
		/*
		** Jn - Julian day, 1 == January 1, 60 == March 1 even in leap
		** years.
		** In non-leap years, or if the day number is 59 or less, just
		** add SECSPERDAY times the day number-1 to the time of
		** January 1, midnight, to get the day.
		*/
		value = (time_t)(janfirst + (rulep->r_day - 1) * SECSPERDAY);
		if (leapyear && rulep->r_day >= 60)
			value += SECSPERDAY;
		break;

	case DAY_OF_YEAR:
		/*
		** n - day of year.
		** Just add SECSPERDAY times the day number to the time of
		** January 1, midnight, to get the day.
		*/
		value = (time_t)(janfirst + rulep->r_day * SECSPERDAY);
		break;

	case MONTH_NTH_DAY_OF_WEEK:
		/*
		** Mm.n.d - nth "dth day" of month m.
		*/
		value = janfirst;
		for (i = 0; i < rulep->r_mon - 1; ++i)
			value += (time_t)(mon_lengths[leapyear][i] * SECSPERDAY);

		/*
		** Use Zeller's Congruence to get day-of-week of first day of
		** month.
		*/
		m1 = (rulep->r_mon + 9) % 12 + 1;
		yy0 = (rulep->r_mon <= 2) ? (year - 1) : year;
		yy1 = yy0 / 100;
		yy2 = yy0 % 100;
		dow = ((26 * m1 - 2) / 10 +
			1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;
		if (dow < 0)
			dow += DAYSPERWEEK;

		/*
		** "dow" is the day-of-week of the first day of the month. Get
		** the day-of-month (zero-origin) of the first "dow" day of the
		** month.
		*/
		d = rulep->r_day - dow;
		if (d < 0)
			d += DAYSPERWEEK;
		for (i = 1; i < rulep->r_week; ++i) {
			if (d + DAYSPERWEEK >=
				mon_lengths[leapyear][rulep->r_mon - 1])
					break;
			d += DAYSPERWEEK;
		}

		/*
		** "d" is the day-of-month (zero-origin) of the day we want.
		*/
		value += (time_t)(d * SECSPERDAY);
		break;
	}

	/*
	** "value" is the Epoch-relative time of 00:00:00 UTC on the day in
	** question. To get the Epoch-relative time of the specified local
	** time on that day, add the transition time and the current offset
	** from UTC.
	*/
	return (time_t)(value + rulep->r_time + offset);
}

/*
** Given a POSIX section 8-style TZ string, fill in the rule tables as
** appropriate.
*/

static int
tzparse(timezone_t sp, const char *name, const int lastditch)
{
	const char *			stdname;
	const char *			dstname;
	size_t				stdlen;
	size_t				dstlen;
	long				stdoffset;
	long				dstoffset;
	time_t *		atp;
	unsigned char *	typep;
	char *			cp;
	int			load_result;

	INITIALIZE(dstname);
	stdname = name;
	if (lastditch) {
		stdlen = strlen(name);	/* length of standard zone name */
		name += stdlen;
		if (stdlen >= sizeof sp->chars)
			stdlen = (sizeof sp->chars) - 1;
		stdoffset = 0;
	} else {
		if (*name == '<') {
			name++;
			stdname = name;
			name = getqzname(name, '>');
			if (*name != '>')
				return (-1);
			stdlen = name - stdname;
			name++;
		} else {
			name = getzname(name);
			stdlen = name - stdname;
		}
		if (*name == '\0')
			return -1;
		name = getoffset(name, &stdoffset);
		if (name == NULL)
			return -1;
	}
	load_result = tzload(sp, TZDEFRULES, FALSE);
	if (load_result != 0)
		sp->leapcnt = 0;		/* so, we're off a little */
	if (*name != '\0') {
		if (*name == '<') {
			dstname = ++name;
			name = getqzname(name, '>');
			if (*name != '>')
				return -1;
			dstlen = name - dstname;
			name++;
		} else {
			dstname = name;
			name = getzname(name);
			dstlen = name - dstname; /* length of DST zone name */
		}
		if (*name != '\0' && *name != ',' && *name != ';') {
			name = getoffset(name, &dstoffset);
			if (name == NULL)
				return -1;
		} else	dstoffset = stdoffset - SECSPERHOUR;
		if (*name == '\0' && load_result != 0)
			name = TZDEFRULESTRING;
		if (*name == ',' || *name == ';') {
			struct rule	start;
			struct rule	end;
			int	year;
			time_t	janfirst;
			time_t		starttime;
			time_t		endtime;

			++name;
			if ((name = getrule(name, &start)) == NULL)
				return -1;
			if (*name++ != ',')
				return -1;
			if ((name = getrule(name, &end)) == NULL)
				return -1;
			if (*name != '\0')
				return -1;
			sp->typecnt = 2;	/* standard time and DST */
			/*
			** Two transitions per year, from EPOCH_YEAR forward.
			*/
			memset(sp->ttis, 0, sizeof(sp->ttis));
			sp->ttis[0].tt_gmtoff = -dstoffset;
			sp->ttis[0].tt_isdst = 1;
			sp->ttis[0].tt_abbrind = (int)(stdlen + 1);
			sp->ttis[1].tt_gmtoff = -stdoffset;
			sp->ttis[1].tt_isdst = 0;
			sp->ttis[1].tt_abbrind = 0;
			atp = sp->ats;
			typep = sp->types;
			janfirst = 0;
			sp->timecnt = 0;
			for (year = EPOCH_YEAR;
			    sp->timecnt + 2 <= TZ_MAX_TIMES;
			    ++year) {
			    	time_t	newfirst;

				starttime = transtime(janfirst, year, &start,
					stdoffset);
				endtime = transtime(janfirst, year, &end,
					dstoffset);
				if (starttime > endtime) {
					*atp++ = endtime;
					*typep++ = 1;	/* DST ends */
					*atp++ = starttime;
					*typep++ = 0;	/* DST begins */
				} else {
					*atp++ = starttime;
					*typep++ = 0;	/* DST begins */
					*atp++ = endtime;
					*typep++ = 1;	/* DST ends */
				}
				sp->timecnt += 2;
				newfirst = janfirst;
				newfirst += (time_t)
				    (year_lengths[isleap(year)] * SECSPERDAY);
				if (newfirst <= janfirst)
					break;
				janfirst = newfirst;
			}
			/*
			** Get zone offsets into tzinfo (for newlib). . .
			*/
			if (sp == lclptr)
			  {
			    __gettzinfo ()->__tzrule[0].offset
						    = -sp->ttis[1].tt_gmtoff;
			    __gettzinfo ()->__tzrule[1].offset
						    = -sp->ttis[0].tt_gmtoff;
			  }
		} else {
			long	theirstdoffset;
			long	theirdstoffset;
			long	theiroffset;
			int	isdst;
			int	i;
			int	j;

			if (*name != '\0')
				return -1;
			/*
			** Initial values of theirstdoffset and theirdstoffset.
			*/
			theirstdoffset = 0;
			for (i = 0; i < sp->timecnt; ++i) {
				j = sp->types[i];
				if (!sp->ttis[j].tt_isdst) {
					theirstdoffset =
						-sp->ttis[j].tt_gmtoff;
					break;
				}
			}
			theirdstoffset = 0;
			for (i = 0; i < sp->timecnt; ++i) {
				j = sp->types[i];
				if (sp->ttis[j].tt_isdst) {
					theirdstoffset =
						-sp->ttis[j].tt_gmtoff;
					break;
				}
			}
			/*
			** Initially we're assumed to be in standard time.
			*/
			isdst = FALSE;
			theiroffset = theirstdoffset;
			/*
			** Now juggle transition times and types
			** tracking offsets as you do.
			*/
			for (i = 0; i < sp->timecnt; ++i) {
				j = sp->types[i];
				sp->types[i] = sp->ttis[j].tt_isdst;
				if (sp->ttis[j].tt_ttisgmt) {
					/* No adjustment to transition time */
				} else {
					/*
					** If summer time is in effect, and the
					** transition time was not specified as
					** standard time, add the summer time
					** offset to the transition time;
					** otherwise, add the standard time
					** offset to the transition time.
					*/
					/*
					** Transitions from DST to DDST
					** will effectively disappear since
					** POSIX provides for only one DST
					** offset.
					*/
					if (isdst && !sp->ttis[j].tt_ttisstd) {
						sp->ats[i] += (time_t)
						    (dstoffset - theirdstoffset);
					} else {
						sp->ats[i] += (time_t)
						    (stdoffset - theirstdoffset);
					}
				}
				theiroffset = -sp->ttis[j].tt_gmtoff;
				if (!sp->ttis[j].tt_isdst)
					theirstdoffset = theiroffset;
				else	theirdstoffset = theiroffset;
			}
			/*
			** Finally, fill in ttis.
			** ttisstd and ttisgmt need not be handled
			*/
			memset(sp->ttis, 0, sizeof(sp->ttis));
			sp->ttis[0].tt_gmtoff = -stdoffset;
			sp->ttis[0].tt_isdst = FALSE;
			sp->ttis[0].tt_abbrind = 0;
			sp->ttis[1].tt_gmtoff = -dstoffset;
			sp->ttis[1].tt_isdst = TRUE;
			sp->ttis[1].tt_abbrind = (int)(stdlen + 1);
			sp->typecnt = 2;
			/*
			** Get zone offsets into tzinfo (for newlib). . .
			*/
			if (sp == lclptr)
			  {
			    __gettzinfo ()->__tzrule[0].offset
						    = -sp->ttis[0].tt_gmtoff;
			    __gettzinfo ()->__tzrule[1].offset
						    = -sp->ttis[1].tt_gmtoff;
			  }
		}
	} else {
		dstlen = 0;
		sp->typecnt = 1;		/* only standard time */
		sp->timecnt = 0;
		memset(sp->ttis, 0, sizeof(sp->ttis));
		sp->ttis[0].tt_gmtoff = -stdoffset;
		sp->ttis[0].tt_isdst = 0;
		sp->ttis[0].tt_abbrind = 0;
		/*
		** Get zone offsets into tzinfo (for newlib). . .
		*/
		if (sp == lclptr)
		  {
		    __gettzinfo ()->__tzrule[0].offset = -sp->ttis[0].tt_gmtoff;
		    __gettzinfo ()->__tzrule[1].offset = -sp->ttis[0].tt_gmtoff;
		  }
	}
	sp->charcnt = (int)(stdlen + 1);
	if (dstlen != 0)
		sp->charcnt += (int)(dstlen + 1);
	if ((size_t) sp->charcnt > sizeof sp->chars)
		return -1;
	cp = sp->chars;
	(void) strncpy(cp, stdname, stdlen);
	cp += stdlen;
	*cp++ = '\0';
	if (dstlen != 0) {
		(void) strncpy(cp, dstname, dstlen);
		*(cp + dstlen) = '\0';
	}
	return 0;
}

static void
gmtload(timezone_t sp)
{
	if (tzload(sp, gmt, TRUE) != 0)
		(void) tzparse(sp, gmt, TRUE);
}

#ifndef STD_INSPIRED
/*
** A non-static declaration of tzsetwall in a system header file
** may cause a warning about this upcoming static declaration...
*/
static
#endif /* !defined STD_INSPIRED */
void
tzsetwall (void)
{
	if (lcl_is_set == lcl_setting)
		return;
	lcl_is_set = lcl_setting;

	if (lclptr == NULL) {
		save_errno save;
		lclptr = (timezone_t) calloc(1, sizeof *lclptr);
		if (lclptr == NULL) {
			settzname();	/* all we can do */
			return;
		}
	}
#if defined (__CYGWIN__)
	{
	    TIME_ZONE_INFORMATION tz;
	    char buf[BUFSIZ];
	    char *cp, *dst;
	    wchar_t *src;
	    div_t d;
	    GetTimeZoneInformation(&tz);
	    dst = cp = buf;
	    for (src = tz.StandardName; *src; src++)
	      if (isupper(*src)) *dst++ = *src;
	    if ((dst - cp) < 3)
	      {
		/* In non-english Windows, converted tz.StandardName
		   may not contain a valid standard timezone name. */
		strcpy(cp, wildabbr);
		cp += strlen(wildabbr);
	      }
	    else
	      cp = dst;
	    d = div(tz.Bias+tz.StandardBias, 60);
	    sprintf(cp, "%d", d.quot);
	    if (d.rem)
		sprintf(cp=strchr(cp, 0), ":%d", abs(d.rem));
	    if(tz.StandardDate.wMonth) {
		cp = strchr(cp, 0);
		dst = cp;
		for (src = tz.DaylightName; *src; src++)
		  if (isupper(*src)) *dst++ = *src;
		if ((dst - cp) < 3)
		  {
		    /* In non-english Windows, converted tz.DaylightName
		       may not contain a valid daylight timezone name. */
		    strcpy(cp, wildabbr);
		    cp += strlen(wildabbr);
		  }
		else
		  cp = dst;
		d = div(tz.Bias+tz.DaylightBias, 60);
		sprintf(cp, "%d", d.quot);
		if (d.rem)
		    sprintf(cp=strchr(cp, 0), ":%d", abs(d.rem));
		cp = strchr(cp, 0);
		sprintf(cp=strchr(cp, 0), ",M%d.%d.%d/%d",
		    tz.DaylightDate.wMonth,
		    tz.DaylightDate.wDay,
		    tz.DaylightDate.wDayOfWeek,
		    tz.DaylightDate.wHour);
		if (tz.DaylightDate.wMinute || tz.DaylightDate.wSecond)
		    sprintf(cp=strchr(cp, 0), ":%d", tz.DaylightDate.wMinute);
		if (tz.DaylightDate.wSecond)
		    sprintf(cp=strchr(cp, 0), ":%d", tz.DaylightDate.wSecond);
		cp = strchr(cp, 0);
		sprintf(cp=strchr(cp, 0), ",M%d.%d.%d/%d",
		    tz.StandardDate.wMonth,
		    tz.StandardDate.wDay,
		    tz.StandardDate.wDayOfWeek,
		    tz.StandardDate.wHour);
		if (tz.StandardDate.wMinute || tz.StandardDate.wSecond)
		    sprintf(cp=strchr(cp, 0), ":%d", tz.StandardDate.wMinute);
		if (tz.StandardDate.wSecond)
		    sprintf(cp=strchr(cp, 0), ":%d", tz.StandardDate.wSecond);
	    }
	    /* printf("TZ deduced as `%s'\n", buf); */
	    if (tzparse(lclptr, buf, FALSE) == 0) {
		settzname();
		lcl_is_set = lcl_from_default;
		strlcpy(lcl_TZname, buf, sizeof (lcl_TZname));
#if 0
		/* Huh?  POSIX doesn't mention anywhere that tzset should
		   set $TZ.  That's not right. */
		setenv("TZ", lcl_TZname, 1);
#endif
		return;
	    }
	}
#endif
	if (tzload(lclptr, NULL, TRUE) != 0)
		gmtload(lclptr);
	settzname();
}

static NO_COPY muto tzset_guard;

#ifndef STD_INSPIRED
/*
** A non-static declaration of tzsetwall in a system header file
** may cause a warning about this upcoming static declaration...
*/
static
#endif /* !defined STD_INSPIRED */
void
tzset_unlocked(void)
{
	const char *	name;

	name = getenv("TZ");
	if (name == NULL) {
		if (lcl_is_set != lcl_from_default)
			tzsetwall();
		return;
	}

	if (lcl_is_set > 0 && strcmp(lcl_TZname, name) == 0)
		return;
	lcl_is_set = (strlen(name) < sizeof (lcl_TZname)) ? lcl_from_environment : lcl_unset;
	if (lcl_is_set != lcl_unset)
		(void)strlcpy(lcl_TZname, name, sizeof (lcl_TZname));

	if (lclptr == NULL) {
		save_errno save;
		lclptr = (timezone_t) calloc(1, sizeof *lclptr);
		if (lclptr == NULL) {
			settzname();	/* all we can do */
			return;
		}
	}
	if (*name == '\0') {
		/*
		** User wants it fast rather than right.
		*/
		lclptr->leapcnt = 0;		/* so, we're off a little */
		lclptr->timecnt = 0;
		lclptr->typecnt = 0;
		lclptr->ttis[0].tt_isdst = 0;
		lclptr->ttis[0].tt_gmtoff = 0;
		lclptr->ttis[0].tt_abbrind = 0;
		(void) strlcpy(lclptr->chars, gmt, sizeof(lclptr->chars));
	} else if (tzload(lclptr, name, TRUE) != 0)
		if (name[0] == ':' || tzparse(lclptr, name, FALSE) != 0)
			(void) gmtload(lclptr);
	settzname();
}

extern "C" void
tzset(void)
{
	tzset_guard.init ("tzset_guard")->acquire ();
	tzset_unlocked();
	tzset_guard.release ();
}

/*
** The easy way to behave "as if no library function calls" localtime
** is to not call it--so we drop its guts into "localsub", which can be
** freely called. (And no, the PANS doesn't require the above behavior--
** but it *is* desirable.)
**
** The unused offset argument is for the benefit of mktime variants.
*/

/*ARGSUSED*/
static struct tm *
localsub(const timezone_t sp, const time_t * const timep, const long offset,
    struct tm *const tmp)
{
	const struct ttinfo *	ttisp;
	int			i;
	struct tm *		result;
	const time_t			t = *timep;

	if ((sp->goback && t < sp->ats[0]) ||
		(sp->goahead && t > sp->ats[sp->timecnt - 1])) {
			time_t			newt = t;
			time_t		seconds;
			time_t		tcycles;
			int_fast64_t	icycles;

			if (t < sp->ats[0])
				seconds = sp->ats[0] - t;
			else	seconds = t - sp->ats[sp->timecnt - 1];
			--seconds;
			tcycles = (time_t)
			    (seconds / YEARSPERREPEAT / AVGSECSPERYEAR);
			++tcycles;
			icycles = tcycles;
			if (tcycles - icycles >= 1 || icycles - tcycles >= 1)
				return NULL;
			seconds = (time_t) icycles;
			seconds *= YEARSPERREPEAT;
			seconds *= AVGSECSPERYEAR;
			if (t < sp->ats[0])
				newt += seconds;
			else	newt -= seconds;
			if (newt < sp->ats[0] ||
				newt > sp->ats[sp->timecnt - 1])
					return NULL;	/* "cannot happen" */
			result = localsub(sp, &newt, offset, tmp);
			if (result == tmp) {
				time_t	newy;

				newy = tmp->tm_year;
				if (t < sp->ats[0])
					newy -= (time_t)icycles * YEARSPERREPEAT;
				else	newy += (time_t)icycles * YEARSPERREPEAT;
				tmp->tm_year = (int)newy;
				if (tmp->tm_year != newy)
					return NULL;
			}
			return result;
	}
	if (sp->timecnt == 0 || t < sp->ats[0]) {
		i = 0;
		while (sp->ttis[i].tt_isdst)
			if (++i >= sp->typecnt) {
				i = 0;
				break;
			}
	} else {
		int	lo = 1;
		int	hi = sp->timecnt;

		while (lo < hi) {
			int	mid = (lo + hi) / 2;

			if (t < sp->ats[mid])
				hi = mid;
			else	lo = mid + 1;
		}
		i = (int) sp->types[lo - 1];
	}
	ttisp = &sp->ttis[i];
	/*
	** To get (wrong) behavior that's compatible with System V Release 2.0
	** you'd replace the statement below with
	**	t += ttisp->tt_gmtoff;
	**	timesub(&t, 0L, sp, tmp);
	*/
	result = timesub(sp, &t, ttisp->tt_gmtoff, tmp);
	tmp->tm_isdst = ttisp->tt_isdst;
	if (sp == lclptr)
		tzname[tmp->tm_isdst] = &sp->chars[ttisp->tt_abbrind];
#ifdef TM_ZONE
	if (CYGWIN_VERSION_CHECK_FOR_EXTRA_TM_MEMBERS)
	  tmp->TM_ZONE = &sp->chars[ttisp->tt_abbrind];
#endif /* defined TM_ZONE */
	return result;
}

/*
** Re-entrant version of localtime.
*/
extern "C" struct tm *
localtime_r(const time_t *__restrict timep, struct tm *__restrict tmp)
{
	tzset_guard.init ("tzset_guard")->acquire ();
	tzset_unlocked();
	tmp = localsub(lclptr, timep, 0L, tmp);
	tzset_guard.release ();
	if (tmp == NULL)
		errno = EOVERFLOW;
	return tmp;
}

extern "C" struct tm *
localtime(const time_t *const timep)
{
	return localtime_r(timep, &tm);
}

/*
** gmtsub is to gmtime as localsub is to localtime.
*/
static NO_COPY muto gmt_guard;

static struct tm *
gmtsub(const timezone_t sp, const time_t *const timep, const long offset,
    struct tm *tmp)
{
	struct tm *	result;

	gmt_guard.init ("gmt_guard")->acquire ();
	if (!gmt_is_set) {
		save_errno save;
		gmt_is_set = TRUE;
		gmtptr = (timezone_t) calloc(1, sizeof *gmtptr);
		if (gmtptr != NULL)
			gmtload(gmtptr);
	}
	gmt_guard.release ();
	result = timesub(gmtptr, timep, offset, tmp);
#ifdef TM_ZONE
	/*
	** Could get fancy here and deliver something such as
	** "UTC+xxxx" or "UTC-xxxx" if offset is non-zero,
	** but this is no time for a treasure hunt.
	*/
	if (CYGWIN_VERSION_CHECK_FOR_EXTRA_TM_MEMBERS)
	  {
	    if (offset != 0)
		    tmp->TM_ZONE = wildabbr;
	    else {
		    if (gmtptr == NULL)
			    tmp->TM_ZONE = gmt;
		    else	tmp->TM_ZONE = gmtptr->chars;
	    }
	  }
#endif /* defined TM_ZONE */
	return result;
}

extern "C" struct tm *
gmtime(const time_t *const timep)
{
	struct tm *tmp = gmtsub(NULL, timep, 0L, &tm);

	if (tmp == NULL)
		errno = EOVERFLOW;

	return tmp;
}

/*
** Re-entrant version of gmtime.
*/

extern "C" struct tm *
gmtime_r(const time_t *__restrict const timep, struct tm *__restrict tmp)
{
	tmp = gmtsub(NULL, timep, 0L, tmp);

	if (tmp == NULL)
		errno = EOVERFLOW;

	return tmp;
}

#ifdef STD_INSPIRED

extern "C" struct tm *
offtime(const time_t *const timep, long offset)
{
	struct tm *tmp = gmtsub(NULL, timep, offset, &tm);

	if (tmp == NULL)
		errno = EOVERFLOW;

	return tmp;
}

#endif /* defined STD_INSPIRED */

/*
** Return the number of leap years through the end of the given year
** where, to make the math easy, the answer for year zero is defined as zero.
*/

static int
leaps_thru_end_of(const int y)
{
	return (y >= 0) ? (y / 4 - y / 100 + y / 400) :
		-(leaps_thru_end_of(-(y + 1)) + 1);
}

static struct tm *
timesub(const timezone_t sp, const time_t *const timep, const long offset,
    struct tm *const tmp)
{
	const struct lsinfo *	lp;
	time_t			tdays;
	int			idays;	/* unsigned would be so 2003 */
	long			rem;
	int			y;
	const int *		ip;
	long			corr;
	int			hit;
	int			i;

	corr = 0;
	hit = 0;
	i = (sp == NULL) ? 0 : sp->leapcnt;
	while (--i >= 0) {
		lp = &sp->lsis[i];
		if (*timep >= lp->ls_trans) {
			if (*timep == lp->ls_trans) {
				hit = ((i == 0 && lp->ls_corr > 0) ||
					lp->ls_corr > sp->lsis[i - 1].ls_corr);
				if (hit)
					while (i > 0 &&
						sp->lsis[i].ls_trans ==
						sp->lsis[i - 1].ls_trans + 1 &&
						sp->lsis[i].ls_corr ==
						sp->lsis[i - 1].ls_corr + 1) {
							++hit;
							--i;
					}
			}
			corr = lp->ls_corr;
			break;
		}
	}
	y = EPOCH_YEAR;
	tdays = (time_t)(*timep / SECSPERDAY);
	rem = (long) (*timep - tdays * SECSPERDAY);
	while (tdays < 0 || tdays >= year_lengths[isleap(y)]) {
		int		newy;
		time_t	tdelta;
		int	idelta;
		int	leapdays;

		tdelta = tdays / DAYSPERLYEAR;
		idelta = (int) tdelta;
		if (tdelta - idelta >= 1 || idelta - tdelta >= 1)
			return NULL;
		if (idelta == 0)
			idelta = (tdays < 0) ? -1 : 1;
		newy = y;
		if (increment_overflow(&newy, idelta))
			return NULL;
		leapdays = leaps_thru_end_of(newy - 1) -
			leaps_thru_end_of(y - 1);
		tdays -= ((time_t) newy - y) * DAYSPERNYEAR;
		tdays -= leapdays;
		y = newy;
	}
	{
		long	seconds;

		seconds = tdays * SECSPERDAY + 0.5;
		tdays = (time_t)(seconds / SECSPERDAY);
		rem += (long) (seconds - tdays * SECSPERDAY);
	}
	/*
	** Given the range, we can now fearlessly cast...
	*/
	idays = (int) tdays;
	rem += offset - corr;
	while (rem < 0) {
		rem += SECSPERDAY;
		--idays;
	}
	while (rem >= SECSPERDAY) {
		rem -= SECSPERDAY;
		++idays;
	}
	while (idays < 0) {
		if (increment_overflow(&y, -1))
			return NULL;
		idays += year_lengths[isleap(y)];
	}
	while (idays >= year_lengths[isleap(y)]) {
		idays -= year_lengths[isleap(y)];
		if (increment_overflow(&y, 1))
			return NULL;
	}
	tmp->tm_year = y;
	if (increment_overflow(&tmp->tm_year, -TM_YEAR_BASE))
		return NULL;
	tmp->tm_yday = idays;
	/*
	** The "extra" mods below avoid overflow problems.
	*/
	tmp->tm_wday = EPOCH_WDAY +
		((y - EPOCH_YEAR) % DAYSPERWEEK) *
		(DAYSPERNYEAR % DAYSPERWEEK) +
		leaps_thru_end_of(y - 1) -
		leaps_thru_end_of(EPOCH_YEAR - 1) +
		idays;
	tmp->tm_wday %= DAYSPERWEEK;
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYSPERWEEK;
	tmp->tm_hour = (int) (rem / SECSPERHOUR);
	rem %= SECSPERHOUR;
	tmp->tm_min = (int) (rem / SECSPERMIN);
	/*
	** A positive leap second requires a special
	** representation. This uses "... ??:59:60" et seq.
	*/
	tmp->tm_sec = (int) (rem % SECSPERMIN) + hit;
	ip = mon_lengths[isleap(y)];
	for (tmp->tm_mon = 0; idays >= ip[tmp->tm_mon]; ++(tmp->tm_mon))
		idays -= ip[tmp->tm_mon];
	tmp->tm_mday = (int) (idays + 1);
	tmp->tm_isdst = 0;
#ifdef TM_GMTOFF
	if (CYGWIN_VERSION_CHECK_FOR_EXTRA_TM_MEMBERS)
	  tmp->TM_GMTOFF = offset;
#endif /* defined TM_GMTOFF */
	return tmp;
}

extern "C" char *
ctime(const time_t *const timep)
{
/*
** Section 4.12.3.2 of X3.159-1989 requires that
**	The ctime function converts the calendar time pointed to by timer
**	to local time in the form of a string. It is equivalent to
**		asctime(localtime(timer))
*/
	struct tm *rtm = localtime(timep);
	if (rtm == NULL)
		return NULL;
	return asctime(rtm);
}

extern "C" char *
ctime_r(const time_t *const timep, char *buf)
{
	struct tm	mytm, *rtm;

	rtm = localtime_r(timep, &mytm);
	if (rtm == NULL)
		return NULL;
	return asctime_r(rtm, buf);
}

/*
** Adapted from code provided by Robert Elz, who writes:
**	The "best" way to do mktime I think is based on an idea of Bob
**	Kridle's (so its said...) from a long time ago.
**	It does a binary search of the time_t space. Since time_t's are
**	just 32 bits, its a max of 32 iterations (even at 64 bits it
**	would still be very reasonable).
*/

#ifndef WRONG
#define WRONG	((time_t)-1)
#endif /* !defined WRONG */

/*
** Simplified normalize logic courtesy Paul Eggert.
*/

static int
increment_overflow(int *const ip, int j)
{
	int	i = *ip;

	/*
	** If i >= 0 there can only be overflow if i + j > INT_MAX
	** or if j > INT_MAX - i; given i >= 0, INT_MAX - i cannot overflow.
	** If i < 0 there can only be overflow if i + j < INT_MIN
	** or if j < INT_MIN - i; given i < 0, INT_MIN - i cannot overflow.
	*/
	if ((i >= 0) ? (j > INT_MAX - i) : (j < INT_MIN - i))
		return TRUE;
	*ip += j;
	return FALSE;
}

static int
long_increment_overflow(long *const lp, int m)
{
	long l = *lp;

	if ((l >= 0) ? (m > LONG_MAX - l) : (m < LONG_MIN - l))
		return TRUE;
	*lp += m;
	return FALSE;
}

static int
normalize_overflow(int *const tensptr, int *const unitsptr, const int base)
{
	int	tensdelta;

	tensdelta = (*unitsptr >= 0) ?
		(*unitsptr / base) :
		(-1 - (-1 - *unitsptr) / base);
	*unitsptr -= tensdelta * base;
	return increment_overflow(tensptr, tensdelta);
}

static int
long_normalize_overflow(long *const tensptr, int *const unitsptr,
    const int base)
{
	int	tensdelta;

	tensdelta = (*unitsptr >= 0) ?
		(*unitsptr / base) :
		(-1 - (-1 - *unitsptr) / base);
	*unitsptr -= tensdelta * base;
	return long_increment_overflow(tensptr, tensdelta);
}

static int
tmcomp(const struct tm *const atmp, const struct tm *const btmp)
{
	int	result;

	if ((result = (atmp->tm_year - btmp->tm_year)) == 0 &&
		(result = (atmp->tm_mon - btmp->tm_mon)) == 0 &&
		(result = (atmp->tm_mday - btmp->tm_mday)) == 0 &&
		(result = (atmp->tm_hour - btmp->tm_hour)) == 0 &&
		(result = (atmp->tm_min - btmp->tm_min)) == 0)
			result = atmp->tm_sec - btmp->tm_sec;
	return result;
}

static time_t
time2sub(const timezone_t sp, struct tm *const tmp, subfun_t funcp,
    const long offset, int *const okayp, const int do_norm_secs)
{
	int			dir;
	int			i, j;
	int			saved_seconds;
	long			li;
	time_t			lo;
	time_t			hi;
#ifdef NO_ERROR_IN_DST_GAP
	time_t			ilo;
#endif
	long				y;
	time_t				newt;
	time_t				t;
	struct tm			yourtm, mytm;

	*okayp = FALSE;
	yourtm = *tmp;
#ifdef NO_ERROR_IN_DST_GAP
again:
#endif
	if (do_norm_secs) {
		if (normalize_overflow(&yourtm.tm_min, &yourtm.tm_sec,
		    SECSPERMIN))
			goto overflow;
	}
	if (normalize_overflow(&yourtm.tm_hour, &yourtm.tm_min, MINSPERHOUR))
		goto overflow;
	if (normalize_overflow(&yourtm.tm_mday, &yourtm.tm_hour, HOURSPERDAY))
		goto overflow;
	y = yourtm.tm_year;
	if (long_normalize_overflow(&y, &yourtm.tm_mon, MONSPERYEAR))
		goto overflow;
	/*
	** Turn y into an actual year number for now.
	** It is converted back to an offset from TM_YEAR_BASE later.
	*/
	if (long_increment_overflow(&y, TM_YEAR_BASE))
		goto overflow;
	while (yourtm.tm_mday <= 0) {
		if (long_increment_overflow(&y, -1))
			goto overflow;
		li = y + (1 < yourtm.tm_mon);
		yourtm.tm_mday += year_lengths[isleap(li)];
	}
	while (yourtm.tm_mday > DAYSPERLYEAR) {
		li = y + (1 < yourtm.tm_mon);
		yourtm.tm_mday -= year_lengths[isleap(li)];
		if (long_increment_overflow(&y, 1))
			goto overflow;
	}
	for ( ; ; ) {
		i = mon_lengths[isleap(y)][yourtm.tm_mon];
		if (yourtm.tm_mday <= i)
			break;
		yourtm.tm_mday -= i;
		if (++yourtm.tm_mon >= MONSPERYEAR) {
			yourtm.tm_mon = 0;
			if (long_increment_overflow(&y, 1))
				goto overflow;
		}
	}
	if (long_increment_overflow(&y, -TM_YEAR_BASE))
		goto overflow;
	yourtm.tm_year = (int)y;
	if (yourtm.tm_year != y)
		goto overflow;
	if (yourtm.tm_sec >= 0 && yourtm.tm_sec < SECSPERMIN)
		saved_seconds = 0;
	else if (y + TM_YEAR_BASE < EPOCH_YEAR) {
		/*
		** We can't set tm_sec to 0, because that might push the
		** time below the minimum representable time.
		** Set tm_sec to 59 instead.
		** This assumes that the minimum representable time is
		** not in the same minute that a leap second was deleted from,
		** which is a safer assumption than using 58 would be.
		*/
		if (increment_overflow(&yourtm.tm_sec, 1 - SECSPERMIN))
			goto overflow;
		saved_seconds = yourtm.tm_sec;
		yourtm.tm_sec = SECSPERMIN - 1;
	} else {
		saved_seconds = yourtm.tm_sec;
		yourtm.tm_sec = 0;
	}
	/*
	** Do a binary search (this works whatever time_t's type is).
	*/
	/* LINTED const not */
	if (!TYPE_SIGNED(time_t)) {
		lo = 0;
		hi = lo - 1;
	/* LINTED const not */
	} else {
		lo = 1;
		for (i = 0; i < (int) TYPE_BIT(time_t) - 1; ++i)
			lo *= 2;
		hi = -(lo + 1);
	}
#ifdef NO_ERROR_IN_DST_GAP
	ilo = lo;
#endif
	for ( ; ; ) {
		t = lo / 2 + hi / 2;
		if (t < lo)
			t = lo;
		else if (t > hi)
			t = hi;
		if ((*funcp)(sp, &t, offset, &mytm) == NULL) {
			/*
			** Assume that t is too extreme to be represented in
			** a struct tm; arrange things so that it is less
			** extreme on the next pass.
			*/
			dir = (t > 0) ? 1 : -1;
		} else	dir = tmcomp(&mytm, &yourtm);
		if (dir != 0) {
			if (t == lo) {
				++t;
				if (t <= lo)
					goto overflow;
				++lo;
			} else if (t == hi) {
				--t;
				if (t >= hi)
					goto overflow;
				--hi;
			}
#ifdef NO_ERROR_IN_DST_GAP
			if (ilo != lo && lo - 1 == hi && yourtm.tm_isdst < 0 &&
			    do_norm_secs) {
				for (i = sp->typecnt - 1; i >= 0; --i) {
					for (j = sp->typecnt - 1; j >= 0; --j) {
						time_t off;
						if (sp->ttis[j].tt_isdst ==
						    sp->ttis[i].tt_isdst)
							continue;
						off = sp->ttis[j].tt_gmtoff -
						    sp->ttis[i].tt_gmtoff;
						yourtm.tm_sec += off < 0 ?
						    -off : off;
						goto again;
					}
				}
			}
#endif
			if (lo > hi)
				goto invalid;
			if (dir > 0)
				hi = t;
			else	lo = t;
			continue;
		}
		if (yourtm.tm_isdst < 0 || mytm.tm_isdst == yourtm.tm_isdst)
			break;
		/*
		** Right time, wrong type.
		** Hunt for right time, right type.
		** It's okay to guess wrong since the guess
		** gets checked.
		*/
		if (sp == NULL)
			goto invalid;
		for (i = sp->typecnt - 1; i >= 0; --i) {
			if (sp->ttis[i].tt_isdst != yourtm.tm_isdst)
				continue;
			for (j = sp->typecnt - 1; j >= 0; --j) {
				if (sp->ttis[j].tt_isdst == yourtm.tm_isdst)
					continue;
				newt = (time_t)(t + sp->ttis[j].tt_gmtoff -
				    sp->ttis[i].tt_gmtoff);
				if ((*funcp)(sp, &newt, offset, &mytm) == NULL)
					continue;
				if (tmcomp(&mytm, &yourtm) != 0)
					continue;
				if (mytm.tm_isdst != yourtm.tm_isdst)
					continue;
				/*
				** We have a match.
				*/
				t = newt;
				goto label;
			}
		}
		goto invalid;
	}
label:
	newt = t + saved_seconds;
	if ((newt < t) != (saved_seconds < 0))
		goto overflow;
	t = newt;
	if ((*funcp)(sp, &t, offset, tmp)) {
		*okayp = TRUE;
		return t;
	}
overflow:
	errno = EOVERFLOW;
	return WRONG;
invalid:
	errno = EINVAL;
	return WRONG;
}

static time_t
time2(const timezone_t sp, struct tm *const tmp, subfun_t funcp,
    const long offset, int *const okayp)
{
	time_t	t;

	/*
	** First try without normalization of seconds
	** (in case tm_sec contains a value associated with a leap second).
	** If that fails, try with normalization of seconds.
	*/
	t = time2sub(sp, tmp, funcp, offset, okayp, FALSE);
	return *okayp ? t : time2sub(sp, tmp, funcp, offset, okayp, TRUE);
}

static time_t
time1(const timezone_t sp, struct tm *const tmp, subfun_t funcp,
    const long offset)
{
	time_t			t;
	int			samei, otheri;
	int			sameind, otherind;
	int			i;
	int			nseen;
	int				seen[TZ_MAX_TYPES];
	int				types[TZ_MAX_TYPES];
	int				okay;

	if (tmp == NULL) {
		errno = EINVAL;
		return WRONG;
	}
	if (tmp->tm_isdst > 1)
		tmp->tm_isdst = 1;
	t = time2(sp, tmp, funcp, offset, &okay);
#ifdef PCTS
	/*
	** PCTS code courtesy Grant Sullivan.
	*/
	if (okay)
		return t;
	if (tmp->tm_isdst < 0)
		tmp->tm_isdst = 0;	/* reset to std and try again */
#endif /* defined PCTS */
#ifndef PCTS
	if (okay || tmp->tm_isdst < 0)
		return t;
#endif /* !defined PCTS */
	/*
	** We're supposed to assume that somebody took a time of one type
	** and did some math on it that yielded a "struct tm" that's bad.
	** We try to divine the type they started from and adjust to the
	** type they need.
	*/
	if (sp == NULL) {
		errno = EINVAL;
		return WRONG;
	}
	for (i = 0; i < sp->typecnt; ++i)
		seen[i] = FALSE;
	nseen = 0;
	for (i = sp->timecnt - 1; i >= 0; --i)
		if (!seen[sp->types[i]]) {
			seen[sp->types[i]] = TRUE;
			types[nseen++] = sp->types[i];
		}
	for (sameind = 0; sameind < nseen; ++sameind) {
		samei = types[sameind];
		if (sp->ttis[samei].tt_isdst != tmp->tm_isdst)
			continue;
		for (otherind = 0; otherind < nseen; ++otherind) {
			otheri = types[otherind];
			if (sp->ttis[otheri].tt_isdst == tmp->tm_isdst)
				continue;
			tmp->tm_sec += (int)(sp->ttis[otheri].tt_gmtoff -
					sp->ttis[samei].tt_gmtoff);
			tmp->tm_isdst = !tmp->tm_isdst;
			t = time2(sp, tmp, funcp, offset, &okay);
			if (okay)
				return t;
			tmp->tm_sec -= (int)(sp->ttis[otheri].tt_gmtoff -
					sp->ttis[samei].tt_gmtoff);
			tmp->tm_isdst = !tmp->tm_isdst;
		}
	}
	errno = EOVERFLOW;
	return WRONG;
}

extern "C" time_t
mktime(struct tm *const tmp)
{
	time_t result;

	tzset_guard.init ("tzset_guard")->acquire ();
	tzset_unlocked();
	result = time1(lclptr, tmp, localsub, 0L);
	tzset_guard.release ();
	return result;
}

#ifdef STD_INSPIRED

extern "C" time_t
timelocal(struct tm *const tmp)
{
	if (tmp != NULL)
		tmp->tm_isdst = -1;	/* in case it wasn't initialized */
	return mktime(tmp);
}

extern "C" time_t
timegm(struct tm *const tmp)
{
	time_t t;

	if (tmp != NULL)
		tmp->tm_isdst = 0;
	t = time1(gmtptr, tmp, gmtsub, 0L);
	return t;
}

extern "C" time_t
timeoff(struct tm *const tmp, const long offset)
{
	time_t t;

	if (tmp != NULL)
		tmp->tm_isdst = 0;
	t = time1(gmtptr, tmp, gmtsub, offset);
	return t;
}

#endif /* defined STD_INSPIRED */

#ifdef CMUCS

/*
** The following is supplied for compatibility with
** previous versions of the CMUCS runtime library.
*/

extern "C" long
gtime(struct tm *const tmp)
{
	const time_t t = mktime(tmp);

	if (t == WRONG)
		return -1;
	return t;
}

#endif /* defined CMUCS */

/*
** XXX--is the below the right way to conditionalize??
*/

#ifdef STD_INSPIRED

/*
** IEEE Std 1003.1-1988 (POSIX) legislates that 536457599
** shall correspond to "Wed Dec 31 23:59:59 UTC 1986", which
** is not the case if we are accounting for leap seconds.
** So, we provide the following conversion routines for use
** when exchanging timestamps with POSIX conforming systems.
*/

static long
leapcorr(const timezone_t sp, time_t *timep)
{
	struct lsinfo * lp;
	int		i;

	i = sp->leapcnt;
	while (--i >= 0) {
		lp = &sp->lsis[i];
		if (*timep >= lp->ls_trans)
			return lp->ls_corr;
	}
	return 0;
}

extern "C" time_t
time2posix(time_t t)
{
	time_t result;
	tzset_guard.init ("tzset_guard")->acquire ();
	tzset_unlocked();
	result = t - leapcorr(lclptr, &t);
	tzset_guard.release ();
	return (result);
}

extern "C" time_t
posix2time(time_t t)
{
	time_t	x;
	time_t	y;

	tzset_guard.init ("tzset_guard")->acquire ();
	tzset_unlocked();
	/*
	** For a positive leap second hit, the result
	** is not unique. For a negative leap second
	** hit, the corresponding time doesn't exist,
	** so we return an adjacent second.
	*/
	x = (time_t)(t + leapcorr(lclptr, &t));
	y = (time_t)(x - leapcorr(lclptr, &x));
	if (y < t) {
		do {
			x++;
			y = (time_t)(x - leapcorr(lclptr, &x));
		} while (y < t);
		if (t != y) {
			return x - 1;
		}
	} else if (y > t) {
		do {
			--x;
			y = (time_t)(x - leapcorr(lclptr, &x));
		} while (y > t);
		if (t != y) {
			return x + 1;
		}
	}
	tzset_guard.release ();
	return x;
}

#endif /* defined STD_INSPIRED */

extern "C" long
__cygwin_gettzoffset (const struct tm *tmp)
{
#ifdef TM_GMTOFF
  if (CYGWIN_VERSION_CHECK_FOR_EXTRA_TM_MEMBERS)
    return tmp->TM_GMTOFF;
#endif /* defined TM_GMTOFF */
  __tzinfo_type *tz = __gettzinfo ();
  /* The sign of this is exactly opposite the envvar TZ.  We
     could directly use the global _timezone for tm_isdst==0,
     but have to use __tzrule for daylight savings.  */
  long offset = -tz->__tzrule[tmp->tm_isdst > 0].offset;
  return offset;
}

extern "C" const char *
__cygwin_gettzname (const struct tm *tmp)
{
#ifdef TM_ZONE
  if (CYGWIN_VERSION_CHECK_FOR_EXTRA_TM_MEMBERS)
    return tmp->TM_ZONE;
#endif
  return _tzname[tmp->tm_isdst > 0];
}
