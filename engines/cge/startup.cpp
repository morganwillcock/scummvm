#include	"startup.h"
#include	"text.h"
#include	"sound.h"
#include	"ident.h"
#include	<cfile.h>
#include	<snddrv.h>
#include	<stdio.h>
#include	<process.h>
#include	<dos.h>
#include	<alloc.h>
#include	<string.h>

#ifdef	DEBUG
  #include	<stdlib.h>
#endif

extern	char	Copr[];

#define	id	(*(IDENT*)Copr)


	EMM	MiniEmm		= MINI_EMM_SIZE;

static	STARTUP	StartUp;


	int	STARTUP::Mode = 0;
	int	STARTUP::Core;
	int	STARTUP::SoundOk = 0;
	word	STARTUP::Summa;



void quit_now (int ref)
{
  fputs(Text[ref], stderr);
  fputc('\n', stderr);
  _exit(1);
}



Boolean STARTUP::get_parms (void)
{
  int i = _argc;
  while (i > 1)
    {
      static char *PrmTab[] = { "NEW", "MK0SVG", "QUIET", "SB", "GUS", "MIDI",
				"P", "D", "I", "M" };
      int n = TakeEnum(PrmTab, strtok(_argv[--i], " =:("));
      word p = xtow(strtok(NULL, " h,)"));
      switch (n)
	{
	  case 0 : if (Mode != 2) Mode = 1; break;
	  case 1 : Mode = 2; break;
	  case 2 : SNDDrvInfo.DDEV = DEV_QUIET; break;
	  case 3 : SNDDrvInfo.DDEV = DEV_SB; break;
	  case 4 : SNDDrvInfo.DDEV = DEV_GUS; break;
	  case 5 : SNDDrvInfo.MDEV = DEV_GM; break;
	  case 6 : SNDDrvInfo.DBASE = p; break;
	  case 7 : SNDDrvInfo.DDMA = p; break;
	  case 8 : SNDDrvInfo.DIRQ = p; break;
	  case 9 : SNDDrvInfo.MBASE = p;
		   SNDDrvInfo.MDEV = DEV_GM; break;
	  default: return FALSE;
	}
      if (n >= 2) SoundOk = 2;
    }
  #ifdef DEMO
      // protection disabled
      Summa = 0;
  #else
    #ifdef EVA
      {
	union { dosdate_t d; dword n; } today;
	_dos_getdate(&today.d);
	id.disk += (id.disk < today.n);
      }
    #endif
    #ifdef CD
      Summa = 0;
    #else
      // disk signature checksum
      Summa = ChkSum(Copr, sizeof(IDENT));
    #endif
  #endif
  if (SNDDrvInfo.MDEV != DEV_GM) SNDDrvInfo.MDEV = SNDDrvInfo.DDEV;
  return TRUE;
}




STARTUP::STARTUP (void)
{
  dword m = farcoreleft() >> 10;
  if (m < 0x7FFF) Core = (int) m; else Core = 0x7FFF;

  if (! IsVga()) quit_now(NOT_VGA_TEXT);
  if (Cpu() < _80286) quit_now(BAD_CHIP_TEXT);
  if (100 * _osmajor + _osminor < 330) quit_now(BAD_DOS_TEXT);

  #ifndef DEBUG
  if (Core < CORE_LOW) quit_now(NO_CORE_TEXT);
  if (Core < CORE_HIG)
    {
      SNDDrvInfo.MDEV = DEV_QUIET;
      Music = FALSE;
    }
  #endif
  if (! get_parms()) quit_now(BAD_ARG_TEXT);
  //--- load sound configuration
  const char * fn = UsrPath(ProgName(CFG_EXT));
  if (! STARTUP::SoundOk && CFILE::Exist(fn))
    {
      CFILE cfg(fn, REA);
      if (! cfg.Error)
	{
	  cfg.Read(&SNDDrvInfo, sizeof(SNDDrvInfo)-sizeof(SNDDrvInfo.VOL2));
	  if (! cfg.Error) STARTUP::SoundOk = 1;
	}
    }
}






const char *UsrPath (const char *nam)
{
  static char buf[MAXPATH] = ".\\", *p = buf+2;
  #if defined(CD)
  if (DriveCD(0))
    {
      Boolean ok = FALSE;
      CFILE ini = Text[CDINI_FNAME];
      if (!ini.Error)
	{
	  char *key = Text[GAME_ID];
	  int i = strlen(key);
	  while (ini.Read(buf) && !ok)
	    {
	      int j = strlen(buf);
	      if (j) if (buf[--j] == '\n') buf[j] = '\0';
	      if (memicmp(buf, key, i) == 0) ok = TRUE;
	    }
	  if (ok)
	    {
	      strcpy(buf, buf+i);
	      p = buf + strlen(buf);
	      if (*(p-1) != '\\') *(p++) = '\\';
	      strcpy(p, "NUL");
	      if (_dos_open(buf, 0, &i) == 0) _dos_close(i);
	      else ok = FALSE;
	    }
	}
      if (!ok) quit_now(BADCD_TEXT);
    }
  #endif
  strcpy(p, nam);
  return buf;
}





